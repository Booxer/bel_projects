/*!
 * @file scu_fg_macros.c
 * @brief Module for handling MIL and non MIL
 *        function generator macros
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author Ulrich Becker <u.becker@gsi.de>
 * @date 04.02.2020
 * Outsourced from scu_main.c
 */
#include "scu_fg_macros.h"
#include "scu_fg_handler.h"

#ifdef CONFIG_SCU_DAQ_INTEGRATION
  #include "daq_main.h"
#endif

extern volatile uint16_t*     g_pScub_base;
#ifdef CONFIG_MIL_FG
extern volatile unsigned int* g_pScu_mil_base;
#endif

/*!
 * @brief Memory space of sent function generator data.
 *        Non shared memory part for each function generator channel.
 */
FG_CHANNEL_T g_aFgChannels[MAX_FG_CHANNELS] = {{0,0}};//,0}};

/*! ---------------------------------------------------------------------------
 * @brief Prints a error message happened in the device-bus respectively
 *        MIL bus.
 * @param status return status of the MIL-driver module.
 * @param slot Slot-number in the case the mil connection is established via
 *             SCU-Bus
 * @param msg String containing additional message text.
 */
void printDeviceError( const int status, const int slot, const char* msg )
{
  static const char* pText = ESC_ERROR"dev bus access in slot ";
  char* pMessage;
  #define __MSG_ITEM( status ) case status: pMessage = #status; break
  switch( status )
  {
     __MSG_ITEM( OKAY );
     __MSG_ITEM( TRM_NOT_FREE );
     __MSG_ITEM( RCV_ERROR );
     __MSG_ITEM( RCV_TIMEOUT );
     __MSG_ITEM( RCV_TASK_ERR );
     default:
     {
        mprintf("%s%d failed with code %d"ESC_NORMAL"\n", pText, slot, status);
        return;
     }
  }
  #undef __MSG_ITEM
  mprintf("%s%d failed with message %s, %s"ESC_NORMAL"\n", pText, slot, pMessage, msg);
}

/*! ---------------------------------------------------------------------------
 * @see scu_fg_macros.h
 * @todo Split this in two separate functions: MIL and non-MIL.
 */
int configure_fg_macro( const unsigned int channel )
{
   if( channel >= MAX_FG_CHANNELS )
      return -1;

   const unsigned int socket = getSocket( channel );
   /* actions per slave card */
#ifdef CONFIG_MIL_FG
   uint16_t dreq_status = 0;

   if( isMilScuBusFg( socket ) )
   {
      scub_status_mil( g_pScub_base, getFgSlotNumber( socket ), &dreq_status );
   }
   else if( isMilExtentionFg( socket ) )
   {
      status_mil( g_pScu_mil_base, &dreq_status );
   }

   #if !defined( CONFIG_GSI ) && !defined( __DOCFSM__ )
    #warning Maybe old Makefile is used, this could be erroneous in using local static variables!
   #endif
   static uint16_t s_clearIsActive = 0;
   STATIC_ASSERT( BIT_SIZEOF( s_clearIsActive ) >= (MAX_SCU_SLAVES + 1) );

   #define _MIL_BIT_MASK  (1 << MAX_SCU_SLAVES)
   #define _SLOT_BIT_MASK() (1 << (getFgSlotNumber( socket ) - 1))

   /*
    * If data request (dreq) is active?
    */
   if( (dreq_status & MIL_DATA_REQ_INTR) != 0 )
   {
      if( isMilScuBusFg( socket ) )
      {
         FG_ASSERT( getFgSlotNumber( socket ) > 0 );
         if( (s_clearIsActive & _SLOT_BIT_MASK()) == 0 )
         {
            s_clearIsActive |= _SLOT_BIT_MASK();
            clear_handler_state( socket );
            hist_addx( HISTORY_XYZ_MODULE, "clear_handler_state", socket );
         }
         return -1;
      }

      if( isMilExtentionFg( socket ) )
      {
         if( (s_clearIsActive & _MIL_BIT_MASK) == 0 )
         {
            s_clearIsActive |= _MIL_BIT_MASK;
            clear_handler_state( socket );
            hist_addx( HISTORY_XYZ_MODULE, "clear_handler_state", socket );
         }
         return -1;
      }
   }
   else
   {  // reset clear
      if( isMilScuBusFg( socket ) )
      {
         FG_ASSERT( getFgSlotNumber( socket ) > 0 );
         s_clearIsActive &= ~_SLOT_BIT_MASK();
      }
      else if( isMilExtentionFg( socket ) )
      {
         s_clearIsActive &= ~_MIL_BIT_MASK;
      }
   }

   #undef _SLOT_BIT_MASK
   #undef _MIL_BIT_MASK
   int status;
#endif /* CONFIG_MIL_FG */

   const unsigned int dev  = getDevice( channel );
    /* enable irqs */
   if( isNonMilFg( socket ) )
   { /*
      * enable irqs for the slave
      */
     scuBusEnableSlaveInterrupt( (void*)g_pScub_base, socket );
     *scuBusGetInterruptActiveFlagRegPtr( (void*)g_pScub_base, socket )  = (FG1_IRQ | FG2_IRQ);
     *scuBusGetInterruptEnableFlagRegPtr( (void*)g_pScub_base, socket ) |= (FG1_IRQ | FG2_IRQ);
   #ifdef CONFIG_SCU_DAQ_INTEGRATION
     daqEnableFgFeedback( socket, dev );
   #endif
   }
#ifdef CONFIG_MIL_FG
   else if( isMilExtentionFg( socket ) )
   {
      if( (status = write_mil(g_pScu_mil_base, 1 << 13, FC_IRQ_MSK | dev)) != OKAY)
         printDeviceError( status, 0, "enable dreq" ); //enable Data-Request
   }
   else if( isMilScuBusFg( socket ) )
   {
      const unsigned int slot = getFgSlotNumber( socket );
      FG_ASSERT( slot > 0 );
      scuBusEnableSlaveInterrupt( (void*)g_pScub_base, slot );
      // enable receiving of drq
      *scuBusGetInterruptEnableFlagRegPtr( (void*)g_pScub_base, slot ) |= DREQ;
      if( (status = scub_write_mil( g_pScub_base, slot, 1 << 13, FC_IRQ_MSK | dev)) != OKAY)
         printDeviceError( status, slot, "enable dreq"); //enable sending of drq
   }
#endif /* CONFIG_MIL_FG */

   unsigned int fg_base = 0;
   /* fg mode and reset */
   FG_REGISTER_T* pAdagFgRegs = NULL;
   if( isNonMilFg( socket ) )
   {   //scu bus slave
      unsigned int dac_base;
      switch( dev )
      {
         case 0:
         {
            fg_base = FG1_BASE;
            dac_base = DAC1_BASE;
            break;
         }
         case 1:
         {
            fg_base = FG2_BASE;
            dac_base = DAC2_BASE;
            break;
         }
         default: return -1;
      }
      pAdagFgRegs = getFgRegisterPtrByOffsetAddr( (void*)g_pScub_base, socket, fg_base );
      g_pScub_base[OFFS(socket) + dac_base + DAC_CNTRL] = 0x10;   // set FG mode
      ADAC_FG_ACCESS( pAdagFgRegs, ramp_cnt_low ) = 0;
      ADAC_FG_ACCESS( pAdagFgRegs, ramp_cnt_high ) = 0;
   }
#ifdef CONFIG_MIL_FG
   else if( isMilExtentionFg( socket ) )
   {
      if( (status = write_mil(g_pScu_mil_base, 0x1, FC_IFAMODE_WR | dev)) != OKAY)
         printDeviceError( status, 0, "set FG mode"); // set FG mode
   }
   else if( isMilScuBusFg( socket ) )
   {
      if( (status = scub_write_mil(g_pScub_base, getFgSlotNumber( socket ), 0x1, FC_IFAMODE_WR | dev)) != OKAY)
         printDeviceError( status, getFgSlotNumber( socket ), "set FG mode"); // set FG mode
   }
#endif


   FG_PARAM_SET_T pset;
   /*
    * Fetch first parameter set from buffer
    */
   if( cbRead(&g_shared.fg_buffer[0], &g_shared.fg_regs[0], channel, &pset) != 0 )
   {
      const uint16_t cntrl_reg_wr = ((pset.control & 0x3F) << 10) | channel << 4;
      if( isNonMilFg( socket ) )
      {
         FG_ASSERT( fg_base != 0 );
         FG_ASSERT( pAdagFgRegs != NULL );

         setAdacFgRegs( pAdagFgRegs, &pset, cntrl_reg_wr );
         STATIC_ASSERT( sizeof( g_shared.fg_regs[channel].tag ) == sizeof( uint32_t ) );
         ADAC_FG_ACCESS( pAdagFgRegs, tag_low )  = g_shared.fg_regs[channel].tag & 0xFFFF;
         ADAC_FG_ACCESS( pAdagFgRegs, tag_high ) = g_shared.fg_regs[channel].tag >> BIT_SIZEOF(uint16_t);
         ADAC_FG_ACCESS( pAdagFgRegs, cntrl_reg.i16 ) |= FG_ENABLED;
      }
   #ifdef CONFIG_MIL_FG
      else
      {
         FG_MIL_REGISTER_T milFgRegs;
         setMilFgRegs( &milFgRegs, &pset, cntrl_reg_wr );
         /*
          * Save the coeff_c as set-value for MIL-DAQ
          */
         g_aFgChannels[channel].last_c_coeff = pset.coeff_c;
         #if __GNUC__ >= 9
           #pragma GCC diagnostic push
           #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
         #endif
         if( isMilExtentionFg( socket ) )
         {
            if((status = write_mil_blk(g_pScu_mil_base, (short*)&milFgRegs, FC_BLK_WR | dev)) != OKAY)
               printDeviceError( status, 0, "blk trm");
           // still in block mode !
            if((status = write_mil(g_pScu_mil_base, cntrl_reg_wr, FC_CNTRL_WR | dev)) != OKAY)
               printDeviceError( status, 0, "end blk trm");

            if( (status = write_mil(g_pScu_mil_base, cntrl_reg_wr | FG_ENABLED, FC_CNTRL_WR | dev)) != OKAY)
               printDeviceError( status, 0, "end blk mode");
         }
         else if( isMilScuBusFg( socket ) )
         {
            if( (status = scub_write_mil_blk(g_pScub_base, getFgSlotNumber( socket ),
                                               (short*)&milFgRegs, FC_BLK_WR | dev)) != OKAY)
               printDeviceError( status, getFgSlotNumber( socket ), "blk trm");

            // still in block mode !
            if( (status = scub_write_mil(g_pScub_base, getFgSlotNumber( socket ),
                                     cntrl_reg_wr, FC_CNTRL_WR | dev)) != OKAY)
               printDeviceError( status, getFgSlotNumber( socket ), "end blk trm");

            if( (status = scub_write_mil( g_pScub_base, getFgSlotNumber( socket ),
                                     cntrl_reg_wr | FG_ENABLED, FC_CNTRL_WR | dev ) ) != OKAY )
               printDeviceError( status, getFgSlotNumber( socket ), "end blk mode");
         }
         #if __GNUC__ >= 9
           #pragma GCC diagnostic pop
         #endif
      }
   #endif /* CONFIG_MIL_FG */
      g_aFgChannels[channel].param_sent++;
   } /* if( cbRead( ... ) != 0 ) */

   // reset watchdog
 //  g_aFgChannels[channel].timeout = 0;
   g_shared.fg_regs[channel].state = STATE_ARMED;
   sendSignal( IRQ_DAT_ARMED, channel );
   return 0;
}

/*! ---------------------------------------------------------------------------
 * @see scu_fg_macros.h
 */
void disable_channel( const unsigned int channel )
{
   FG_CHANNEL_REG_T* pFgRegs = &g_shared.fg_regs[channel];

   if( pFgRegs->macro_number == SCU_INVALID_VALUE )
      return;

#ifdef CONFIG_MIL_FG
   int status;
   int16_t data;
#endif
   const uint8_t socket = getSocket( channel );
   const uint16_t dev   = getDevice( channel );
   //mprintf("disarmed socket %d dev %d in channel[%d] state %d\n", socket, dev, channel, pFgRegs->state); //ONLY FOR TESTING
   if( isNonMilFg( socket ) )
   {
      unsigned int fg_base, dac_base;
      /* which macro are we? */
      switch( dev )
      {
         case 0:
         {
            fg_base = FG1_BASE;
            dac_base = DAC1_BASE;
            break;
         }
         case 1:
         {
            fg_base = FG2_BASE;
            dac_base = DAC2_BASE;
            break;
         }
         default: return;
      }
   #ifdef CONFIG_SCU_DAQ_INTEGRATION
      daqDisableFgFeedback( socket, dev );
   #endif
     // disarm hardware
      g_pScub_base[OFFS(socket) + fg_base + FG_CNTRL] &= ~(0x2);
      g_pScub_base[OFFS(socket) + dac_base + DAC_CNTRL] &= ~(0x10); // unset FG mode
   }
#ifdef CONFIG_MIL_FG
   else if( isMilExtentionFg( socket ) )
   {  // disarm hardware
      if( (status = read_mil( g_pScu_mil_base, &data, FC_CNTRL_RD | dev)) != OKAY )
         printDeviceError( status, 0, "disarm hw 1" );

      if( (status = write_mil( g_pScu_mil_base, data & ~(0x2), FC_CNTRL_WR | dev)) != OKAY )
         printDeviceError( status, 0, "disarm hw 2" );
   }
   else if( isMilScuBusFg( socket ) )
   {  // disarm hardware
      if( (status = scub_read_mil( g_pScub_base, getFgSlotNumber( socket ),
           &data, FC_CNTRL_RD | dev)) != OKAY )
         printDeviceError( status, getFgSlotNumber( socket ), "disarm hw 3" );

      if( (status = scub_write_mil( g_pScub_base, getFgSlotNumber( socket ),
           data & ~(0x2), FC_CNTRL_WR | dev)) != OKAY )
         printDeviceError( status, getFgSlotNumber( socket ), "disarm hw 4" );
   }
#endif /* CONFIG_MIL_FG */

   if( pFgRegs->state == STATE_ACTIVE )
   {    // hw is running
      hist_addx( HISTORY_XYZ_MODULE, "flush circular buffer", channel );
      pFgRegs->rd_ptr =  pFgRegs->wr_ptr;
   }
   else
   {
      pFgRegs->state = STATE_STOPPED;
      sendSignal( IRQ_DAT_DISARMED, channel );
   }
}
/*! ---------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief disables the generation of irqs for the specified channel
 *  SIO and MIL extension stop generating irqs
 *  @param channel number of the channel from 0 to MAX_FG_CHANNELS-1
 * @see enable_scub_msis
 */
void disable_slave_irq( const unsigned int channel )
{
   if( channel >= MAX_FG_CHANNELS )
      return;

   const unsigned int socket = getSocket( channel );
   const unsigned int dev    = getDevice( channel );

   //mprintf("IRQs for slave %d disabled.\n", socket);

   if( isNonMilFg( socket ) )
   {
#if 0
      if( dev == 0 )
        g_pScub_base[OFFS(socket) + SLAVE_INT_ENA] &= ~FG1_IRQ; //disable fg1 irq
      else if( dev == 1 )
        g_pScub_base[OFFS(socket) + SLAVE_INT_ENA] &= ~FG2_IRQ; //disable fg2 irq
#else
      *scuBusGetInterruptEnableFlagRegPtr( (void*)g_pScub_base, socket ) &=
                                            ((dev == 0)? ~FG1_IRQ : ~FG2_IRQ);
#endif
      return;
   }

#ifdef CONFIG_MIL_FG
   int status = OKAY;

   if( isMilExtentionFg( socket ) )
   {
      //write_mil(g_pScu_mil_base, 0x0, FC_COEFF_A_WR | dev);            //ack drq
      status = write_mil( g_pScu_mil_base, 0x0, FC_IRQ_MSK | dev);
   }
   else if( isMilScuBusFg( socket ) )
   {
      status = scub_write_mil( g_pScub_base, getFgSlotNumber( socket ),
                                   0x0, FC_IRQ_MSK | dev);
   }
   if( status != OKAY )
      printDeviceError( status, getFgSlotNumber( socket ), __func__);
#endif /* ifdef CONFIG_MIL_FG */
}

/*! ---------------------------------------------------------------------------
 * @brief Send signal REFILL to the SAFTLIB when the fifo level has
 *        the threshold reached. Helper function of function handleMacros().
 * @see handleMacros
 * @param channel Channel of concerning function generator.
 */
void sendRefillSignalIfThreshold( const unsigned int channel )
{
   if( cbgetCount( &g_shared.fg_regs[0], channel ) == THRESHOLD )
   {
      //mprintf( "*" );
      sendSignal( IRQ_DAT_REFILL, channel );
   }
}

/*================================== EOF ====================================*/