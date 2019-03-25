/*!
 *  @file daq.c
 *  @brief Control module for Data Acquisition Unit (DAQ)
 *  @see
 *  <a href="https://www-acc.gsi.de/wiki/Hardware/Intern/DataAquisitionMacrof%C3%BCrSCUSlaveBaugruppen">
 *     Data Aquisition Macro fuer SCU Slave Baugruppen</a>
 *  @date 13.11.2018
 *  @copyright (C) 2018 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Ulrich Becker <u.becker@gsi.de>
 *
 *  @todo Synchronization with SCU-Bus. It could be there that further devices
 *        which have traffic via this SCU-Bus!
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#include <string.h>   // necessary for memset()
#include <mini_sdb.h> // necessary for ERROR_NOT_FOUND
#include <dbg.h>
#include <daq.h>
#ifdef CONFIG_DAQ_DEBUG
 #include <eb_console_helper.h> //!@brief Will used for debug purposes only.
#endif

#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
//! @brief For debug purposes only
const char* g_pYes = ESC_FG_WHITE ESC_BOLD"yes"ESC_NORMAL;
//! @brief For debug purposes only
const char* g_pNo  = "no";
#endif


/*======================== DAQ channel functions ============================*/
#if 1
/*! ---------------------------------------------------------------------------
 * @brief Writes the given value in addressed register
 * @param pReg Start address of DAQ-macro.
 * @param index Offset address of register @see
 * @param channel Channel number.
 * @param value Value for writing into register.
 */
static inline void daqChannelSetReg( DAQ_REGISTER_T* volatile pReg,
                                     const DAQ_REGISTER_INDEX_T index,
                                     const unsigned int channel,
                                     const uint16_t value )
{
   DAQ_ASSERT( channel < DAQ_MAX_CHANNELS );
   DAQ_ASSERT( (index & 0x0F) == 0x00 );
   pReg->i[index | channel] = value;
}

/*! ---------------------------------------------------------------------------
 * @brief Reads a value from a addressed register
 * @param pReg Start address of DAQ-macro.
 * @param index Offset address of register @see
 * @param channel Channel number.
 * @return Register value.
 */
static inline uint16_t daqChannelGetReg( DAQ_REGISTER_T* volatile pReg,
                                         const DAQ_REGISTER_INDEX_T index,
                                         const unsigned int channel )
{
   DAQ_ASSERT( channel < DAQ_MAX_CHANNELS );
   DAQ_ASSERT( (index & 0x0F) == 0x00 );
   return pReg->i[index | channel];
}
#endif
#ifndef CONFIG_DAQ_SIMULATE_CHANNEL
/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqChannelReset( register DAQ_CANNEL_T* pThis )
{
   daqChannelSample10usOff( pThis );
   daqChannelSample100usOff( pThis );
   daqChannelSample1msOff( pThis );
   daqChannelDisableTriggerMode( pThis );
   daqChannelEnableEventTrigger( pThis );
   daqChannelDisablePostMortem( pThis );
   daqChannelDisableHighResolution( pThis );
   daqChannelEnableEventTriggerHighRes( pThis );
   daqChannelSetTriggerConditionLW( pThis, 0 );
   daqChannelSetTriggerConditionHW( pThis, 0 );
   daqChannelSetTriggerDelay( pThis, 0 );

   unsigned int i;
   volatile uint16_t dummy;
   /*
    * Making the PM_HiRes Fifo empty and discard the content
    */
   for( i = 0; i < DAQ_FIFO_PM_HIRES_WORD_SIZE; i++ )
      dummy = daqChannelPopPmFifo( pThis );

   /*
    * Making the DAQ Fifo empty and discard the content
    */
   for( i = 0; i < DAQ_FIFO_DAQ_WORD_SIZE; i++ )
      dummy = daqChannelPopDaqFifo( pThis );

   daqChannelTestAndClearDaqIntPending( pThis );
   daqChannelTestAndClearHiResIntPending( pThis );
}
#else /* ifndef CONFIG_DAQ_SIMULATE_CHANNEL */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * CAUTION: Following functions are for simulation purposes only!
 */
/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqChannelReset( register DAQ_CANNEL_T* pThis )
{
   memset( pThis, 0x00, sizeof( DAQ_CANNEL_T ) );
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
unsigned int daqChannelGetPmFifoWordsSimulate( register DAQ_CANNEL_T* pThis )
{
   return DAQ_FIFO_PM_HIRES_WORD_SIZE - pThis->callCount;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
unsigned int daqChannelGetDaqFifoWordsSimulate( register DAQ_CANNEL_T* pThis )
{
   return DAQ_FIFO_DAQ_WORD_SIZE  - pThis->callCount;
}

/*! ---------------------------------------------------------------------------
 * @brief Simulates a DAQ single shot stream finalized by a fake
 *        DAQ-descriptor. The payload becomes simulated by increasing numbers.
 *
 * The time stamp counter will misused as periodic counter.
 * @note CAUTION: This function is for developing and testing purposes only and
 *                becomes compiled if the compiler-switch
 *                CONFIG_DAQ_SIMULATE_CHANNEL defined!
 */
static
DAQ_DATA_T daqChannelPopFifoSimulate( register DAQ_CANNEL_T* pThis,
                                      unsigned int remaining,
                                      const unsigned int limit )
{
   DAQ_DATA_T ret;

   if( pThis->callCount > limit )
      return 0;

   if( remaining >= ARRAY_SIZE( pThis->simulatedDescriptor.index ) )
      ret = pThis->callCount + 1;
   else
   {
      unsigned int i = ARRAY_SIZE( pThis->simulatedDescriptor.index ) - 1
                       - remaining;
      ret = pThis->simulatedDescriptor.index[i];
      DBPRINT2( "DBG: i: %d ret: 0x%04x\n", i, ret );
   }

   if( pThis->callCount < limit )
      pThis->callCount++;
   else
   {
      pThis->callCount = 0;
      pThis->simulatedDescriptor.name.wr.name.utSec++;
   }

   return ret;
}

/*! ---------------------------------------------------------------------------
 * @brief Simulates a DAQ single shot stream finalized by a fake
 *        DAQ-descriptor. The payload becomes simulated by increasing numbers.
 *
 * The time stamp counter will misused as periodic counter.
 * @note CAUTION: This function is for developing and testing purposes only and
 *                becomes compiled if the compiler-switch
 *                CONFIG_DAQ_SIMULATE_CHANNEL defined!
 * @param pThis Pointer to the channel object
 * @return Simulated fake data.
 */
DAQ_DATA_T daqChannelPopPmFifoSimulate( register DAQ_CANNEL_T* pThis )
{
   return daqChannelPopFifoSimulate( pThis,
                                     daqChannelGetPmFifoWordsSimulate( pThis ),
                                     DAQ_FIFO_PM_HIRES_WORD_SIZE );
}

/*! ---------------------------------------------------------------------------
 * @brief Simulates a DAQ continuous stream finalized by a fake
 *        DAQ-descriptor. The payload becomes simulated by increasing numbers.
 *
 * The time stamp counter will misused as periodic counter.
 * @note CAUTION: This function is for developing and testing purposes only and
 *                becomes compiled if the compiler-switch
 *                CONFIG_DAQ_SIMULATE_CHANNEL defined!
 * @param pThis Pointer to the channel object
 * @return Simulated fake data.
 */
DAQ_DATA_T daqChannelPopDaqFifoSimulate( register DAQ_CANNEL_T* pThis )
{
   return daqChannelPopFifoSimulate( pThis,
                                     daqChannelGetDaqFifoWordsSimulate( pThis ),
                                     DAQ_FIFO_DAQ_WORD_SIZE );
}

/*
 * End of prototypes for simulation!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
#endif /* ifdef CONFIG_DAQ_SIMULATE_CHANNEL */

#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqChannelPrintInfo( register DAQ_CANNEL_T* pThis )
{
   mprintf( ESC_BOLD ESC_FG_CYAN
            "Slot: %d, Channel %d, Address: 0x%08x, Bus address: 0x%08x\n"
            ESC_NORMAL,
            daqChannelGetSlot( pThis ),
            daqChannelGetNumber( pThis ) + 1,
            daqChannelGetRegPtr( pThis ),
            daqChannelGetScuBusSlaveBaseAddress( pThis )
          );
   const uint16_t ctrlReg = *(uint16_t*)daqChannelGetCtrlRegPtr( pThis );
   mprintf( "  CtrlReg: &0x%08x *0x%04x *0b",
            daqChannelGetCtrlRegPtr( pThis ), ctrlReg );

   for( uint16_t i = 1 << (BIT_SIZEOF(uint16_t) - 1); i != 0; i >>= 1 )
      mprintf( "%c", (ctrlReg & i)? '1' : '0' );

   mprintf( "\n    Ena_PM:                %s\n",
            daqChannelIsPostMortemActive( pThis )? g_pYes : g_pNo );
   mprintf( "    Sample10us:            %s\n",
            daqChannelIsSample10usActive( pThis )? g_pYes : g_pNo );
   mprintf( "    Sample100us:           %s\n",
            daqChannelIsSample100usActive( pThis )? g_pYes : g_pNo );
   mprintf( "    Sample1ms:             %s\n",
            daqChannelIsSample1msActive( pThis )? g_pYes : g_pNo );
   mprintf( "    Ena_TrigMod:           %s\n",
            daqChannelIsTriggerModeEnabled( pThis )? g_pYes : g_pNo );
   mprintf( "    ExtTrig_nEvTrig:       %s\n",
            daqChannelGetTriggerSource( pThis )?  g_pYes : g_pNo );
   mprintf( "    Ena_HiRes:             %s\n",
            daqChannelIsHighResolutionEnabled( pThis )? g_pYes : g_pNo );
   mprintf( "    ExtTrig_nEvTrig_HiRes: %s\n",
            daqChannelGetTriggerSourceHighRes( pThis )? g_pYes : g_pNo );
   mprintf( "  Trig_LW:  &0x%08x *0x%04x\n",
            &__DAQ_GET_CHANNEL_REG( TRIG_LW ),
            daqChannelGetTriggerConditionLW( pThis ) );
   mprintf( "  Trig_HW:  &0x%08x *0x%04x\n",
            &__DAQ_GET_CHANNEL_REG( TRIG_HW ),
            daqChannelGetTriggerConditionHW( pThis ) );
   mprintf( "  Trig_Dly: &0x%08x *0x%04x\n",
            &__DAQ_GET_CHANNEL_REG( TRIG_DLY ),
            daqChannelGetTriggerDelay( pThis ) );
   mprintf( "  DAQ int pending:     %s\n",
         (((*daqChannelGetDaqIntPendingPtr( pThis )) & pThis->intMask) != 0 )?
         g_pYes : g_pNo );
   mprintf( "  HiRes int pending:   %s\n",
       (((*daqChannelGetHiResIntPendingPtr( pThis )) & pThis->intMask) != 0 )?
         g_pYes : g_pNo );
   mprintf( "  VHDL Macro version:  %d\n",
            daqChannelGetMacroVersion( pThis ));
   mprintf( "  Level DAQ FiFo:      %d words\n",
            daqChannelGetDaqFifoWords( pThis ));
   mprintf( "  Level PM_HiRes FiFo: %d words \n",
            daqChannelGetPmFifoWords( pThis ));
   mprintf( "  Channels:            %d\n",
            daqChannelGetMaxCannels( pThis ));
}
#endif /* defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__) */

/*======================== DAQ- Device Functions ============================*/

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
unsigned int daqDeviceGetUsedChannels( register DAQ_DEVICE_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   unsigned int retVal = 0;

   for( int i = daqDeviceGetMaxChannels( pThis )-1; i >= 0; i-- )
   {
      if( !daqDeviceGetChannelObject( pThis, i )->properties.notUsed )
         retVal++;
   }
   return retVal;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqDeviceSetTimeStampCounter( register DAQ_DEVICE_T* pThis, uint64_t ts )
{
   DAQ_ASSERT( pThis != NULL );
   DAQ_ASSERT( pThis->pReg != NULL );

   for( unsigned int i = 0; i < (sizeof(uint64_t)/sizeof(uint16_t)); i++ )
      pThis->pReg->i[TS_COUNTER_WD1+i] = ((uint16_t*)&ts)[i];
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
uint64_t daqDeviceGetTimeStampCounter( register DAQ_DEVICE_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   DAQ_ASSERT( pThis->pReg != NULL );

   uint64_t ts;

   for( unsigned int i = 0; i < (sizeof(uint64_t)/sizeof(uint16_t)); i++ )
      ((uint16_t*)&ts)[i] = pThis->pReg->i[TS_COUNTER_WD1+i];

   return ts;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqDeviceSetTimeStampTag( register DAQ_DEVICE_T* pThis, uint32_t tsTag )
{
   DAQ_ASSERT( pThis != NULL );
   DAQ_ASSERT( pThis->pReg != NULL );

   for( unsigned int i = 0; i < (sizeof(uint32_t)/sizeof(uint16_t)); i++ )
      pThis->pReg->i[TS_CNTR_TAG_LW+i] = ((uint16_t*)&tsTag)[i];
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
uint32_t daqDeviceGetTimeStampTag( register DAQ_DEVICE_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   DAQ_ASSERT( pThis->pReg != NULL );

   uint32_t tsTag;

   for( unsigned int i = 0; i < (sizeof(uint32_t)/sizeof(uint16_t)); i++ )
      ((uint16_t*)&tsTag)[i] = pThis->pReg->i[TS_CNTR_TAG_LW+i];

   return tsTag;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqDeviceReset( register DAQ_DEVICE_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   DAQ_ASSERT( pThis->pReg != NULL );

   for( int i = daqDeviceGetMaxChannels( pThis )-1; i >= 0; i-- )
      daqChannelReset( daqDeviceGetChannelObject( pThis, i ) );

   daqDeviceSetTimeStampCounter( pThis, 0L );
   daqDeviceSetTimeStampTag( pThis, 0 );
}

#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqDevicePrintInfo( register DAQ_DEVICE_T* pThis )
{
   mprintf( "Macro version: %d\n", daqDeviceGetMacroVersion( pThis ) );
   unsigned int maxChannels = daqDeviceGetMaxChannels( pThis );
   for( unsigned int i = 0; i < maxChannels; i++ )
      daqChannelPrintInfo( daqDeviceGetChannelObject( pThis, i ));
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqDevicePrintInterruptStatus( register DAQ_DEVICE_T* pThis )
{
   uint16_t flags = scuBusGetSlaveValue16(
                   daqDeviceGetScuBusSlaveBaseAddress( pThis ), Intr_Active );
   mprintf( "SCU slave DAQ interrupt active:   %s\n",
            ((flags & (1 << DAQ_IRQ_DAQ_FIFO_FULL)) != 0)? g_pYes : g_pNo);
   mprintf( "SCU slave HiRes interrupt active: %s\n",
            ((flags & (1 << DAQ_IRQ_HIRES_FINISHED)) != 0)? g_pYes : g_pNo);
}

#endif /* defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__) */


/*! ---------------------------------------------------------------------------
 * @ingroup DAQ_DEVICE
 * @brief Scans all potential existing input-channels of the given
 *        DAQ-Device ant initialize each found channel with
 *        the slot number.
 * @param pThis Pointer to the DAQ device object
 * @param slot slot number
 * @return Number of real existing channels
 */
inline static int daqDeviceFindChannels( DAQ_DEVICE_T* pThis, int slot )
{
   DAQ_ASSERT( pThis != NULL );
   DAQ_ASSERT( pThis->pReg != NULL );

   for( unsigned int channel = 0; channel < ARRAY_SIZE(pThis->aChannel); channel++ )
   {
      DBPRINT2( "DBG: Slot: %02d, Channel: %02d, ctrlReg: 0x%04x\n",
                slot, channel, daqChannelGetReg( pThis->pReg, CtrlReg, channel ));

      DAQ_CANNEL_T* pCurrentChannel = &pThis->aChannel[channel];
      pCurrentChannel->n = channel;

      /*
       * Checking whether the macro DAQ_CHANNEL_GET_PARENT_OF works
       * correct.
       */
      DAQ_ASSERT( pThis == DAQ_CHANNEL_GET_PARENT_OF( pCurrentChannel ) );

      /*
       * In the case of a warm start, clearing eventually old values
       * in the entire control register.
       */
      *((uint16_t*)daqChannelGetCtrlRegPtr( pCurrentChannel )) = 0;

      /*
       * The next three lines probes the channel by writing and read back
       * the slot number. At the first look this algorithm seems not meaningful
       * (writing and immediately reading a value from the same memory place)
       * but we have to keep in mind that is a memory mapped IO areal and
       * its attribute was declared as "volatile".
       *
       * If no (further) channel present the value of the control-register is
       * 0xDEAD. That means the slot number has the hex number 0xD (13).
       * Fortunately the highest slot number is 0xC (12). Therefore no further
       * probing is necessary.
       */
      DBPRINT2( "DBG: ctrReg: 0x%04x\n", *((uint16_t*)daqChannelGetCtrlRegPtr( pCurrentChannel )) );
      daqChannelGetCtrlRegPtr( pCurrentChannel )->slot = slot;
      DBPRINT2( "DBG: ctrReg: 0x%04x\n", *((uint16_t*)daqChannelGetCtrlRegPtr( pCurrentChannel )) );
      if( daqChannelGetSlot( pCurrentChannel ) != slot )
         break; /* Supposing this channel isn't present. */

      DAQ_ASSERT( (*((uint16_t*)daqChannelGetCtrlRegPtr( pCurrentChannel )) & 0x0FFF) == 0 );
      /* If the assertion above has been occurred, check the element types
       * of the structure DAQ_CTRL_REG_T.
       * At least one element type has to be greater or equal like uint16_t.
       */

      DAQ_ASSERT( channel < BIT_SIZEOF( pCurrentChannel->intMask ));
      pCurrentChannel->intMask = 1 << channel;

      DBPRINT2( "DBG: Slot of channel %d: %d\n", channel,
                daqChannelGetSlot( pCurrentChannel ) );

      pThis->maxChannels++;
   }
   return pThis->maxChannels;
}

/*============================ DAQ Bus Functions ============================*/
#ifndef CONFIG_DAQ_SIMULATE_CHANNEL
/*! ----------------------------------------------------------------------------
 * @see daq.h
 */
int daqBusFindAndInitializeAll( register DAQ_BUS_T* pThis,
                                const void* pScuBusBase )
{
   // Paranoia...
   DAQ_ASSERT( pScuBusBase != (void*)ERROR_NOT_FOUND );
   DAQ_ASSERT( pThis != NULL );

   // Pre-initializing
   memset( pThis, 0, sizeof( DAQ_BUS_T ));

   // Find all DAQ- slaves
   pThis->slotDaqUsedFlags = scuBusFindSpecificSlaves( pScuBusBase,
                                                       DAQ_CID_SYS,
                                                       DAQ_CID_GROUP );
   if( pThis->slotDaqUsedFlags == 0 )
   {
      DBPRINT( "DBG: No DAQ slaves found!\n" );
      return 0;
   }

   for( int slot = SCUBUS_START_SLOT; slot <= MAX_SCU_SLAVES; slot++ )
   {
      if( !scuBusIsSlavePresent( pThis->slotDaqUsedFlags, slot ) )
         continue; /* In this slot is not a DAQ! */

      /*
       * For each found DAQ-device:
       */
      DAQ_DEVICE_T* pCurrentDaqDevice = &pThis->aDaq[pThis->foundDevices];
      pCurrentDaqDevice->n = pThis->foundDevices;
     /*
      * Because the register access to the DAQ device is more frequent than
      * to the registers of the SCU slave, therefore the base address of the
      * DAQ registers are noted rather than the SCU bus slave address.
      */
      pCurrentDaqDevice->pReg =
          scuBusGetAbsSlaveAddr( pScuBusBase, slot ) + DAQ_REGISTER_OFFSET;

      DBPRINT2( "DBG: DAQ found in slot: %02d, address: 0x%08x\n", slot,
                pCurrentDaqDevice->pReg );


      if( daqDeviceFindChannels( pCurrentDaqDevice, slot ) == 0 )
      {
         DBPRINT2( "DBG: DAQ in slot %d has no input channels - skipping\n", slot );
         continue;
      }
      pThis->foundDevices++; // At least one channel was found.

      DAQ_ASSERT( pCurrentDaqDevice->maxChannels ==
                   daqDeviceGetMaxChannels( pCurrentDaqDevice ) );
      DAQ_ASSERT( DAQ_DEVICE_GET_PARENT_OF( pCurrentDaqDevice ) == pThis );

      //daqDeviceDisableScuSlaveInterrupt( pCurrentDaqDevice );
      daqDeviceEnableScuSlaveInterrupt( pCurrentDaqDevice ); //!!
      daqDeviceTestAndClearDaqInt( pCurrentDaqDevice );
      daqDeviceTestAndClearHiResInt( pCurrentDaqDevice );

      daqDeviceClearDaqChannelInterrupts( pCurrentDaqDevice );
      daqDeviceClearHiResChannelInterrupts( pCurrentDaqDevice );

#if DAQ_MAX < MAX_SCU_SLAVES
      if( pThis->foundDevices == ARRAY_SIZE( pThis->aDaq ) )
         break;
#endif
   }

   /*
    * In the case of re-initializing respectively warm-start a
    * reset for all DAQ devices becomes necessary.
    * Because a new start of the software doesn't concern
    * the hardware registers of the SCU bus slaves.
    */

   if( pThis->foundDevices > 0 )
      daqBusReset( pThis );

   return pThis->foundDevices;
}

#endif

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
int daqBusGetNumberOfAllFoundChannels( register DAQ_BUS_T* pThis )
{
   int ret = 0;
   DAQ_ASSERT( pThis->foundDevices <= ARRAY_SIZE(pThis->aDaq) );
   for( int i = 0; i < pThis->foundDevices; i++ )
      ret += pThis->aDaq[i].maxChannels;
   return ret;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
DAQ_DEVICE_T* daqBusGetDeviceBySlotNumber( register DAQ_BUS_T* pThis,
                                           unsigned int slot )
{
   for( unsigned int i = 0; i < pThis->foundDevices; i++ )
   {
      DAQ_DEVICE_T* pDevice = daqBusGetDeviceObject( pThis, i );
      if( slot == daqDeviceGetSlot( pDevice ) )
         return pDevice;
   }
   return NULL;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
DAQ_CANNEL_T* daqBusGetChannelObjectByAbsoluteNumber( register DAQ_BUS_T* pThis,
                                                      const unsigned int n )
{
   unsigned int deviceCounter;
   unsigned int relativeChannelCounter;
   unsigned int absoluteChannelCounter = 0;

   for( deviceCounter = 0; deviceCounter < daqBusGetFoundDevices(pThis); deviceCounter++ )
   {
      DAQ_DEVICE_T* pDevice = daqBusGetDeviceObject( pThis, deviceCounter );
      for( relativeChannelCounter = 0;
           relativeChannelCounter < daqDeviceGetMaxChannels( pDevice );
           relativeChannelCounter++ )
      {
         if( absoluteChannelCounter == n )
            return daqDeviceGetChannelObject( pDevice, relativeChannelCounter );
         absoluteChannelCounter++;
      }
   }
   return NULL;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
unsigned int daqBusGetUsedChannels( register DAQ_BUS_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   unsigned int retVal = 0;

   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
   {
      retVal += daqDeviceGetUsedChannels( daqBusGetDeviceObject( pThis, i ) );
   }
   return retVal;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusEnableSlaveInterrupts( register DAQ_BUS_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
      daqDeviceEnableScuSlaveInterrupt( daqBusGetDeviceObject( pThis, i ) );
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusDisablSlaveInterrupts( register DAQ_BUS_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );
   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
      daqDeviceDisableScuSlaveInterrupt( daqBusGetDeviceObject( pThis, i ) );
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusClearAllPendingInterrupts( register DAQ_BUS_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );

   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
   {
      DAQ_DEVICE_T* pDaqSlave = daqBusGetDeviceObject( pThis, i );
      daqDeviceClearDaqChannelInterrupts( pDaqSlave );
      daqDeviceClearHiResChannelInterrupts( pDaqSlave );
   }
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusSetAllTimeStampCounters( register DAQ_BUS_T* pThis, uint64_t ts )
{
   DAQ_ASSERT( pThis != NULL );

   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
      daqDeviceSetTimeStampCounter( daqBusGetDeviceObject( pThis, i ), ts );
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusSetAllTimeStampCounterTags( register DAQ_BUS_T* pThis, uint32_t tsTag )
{
   DAQ_ASSERT( pThis != NULL );
   
   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
      daqDeviceSetTimeStampTag( daqBusGetDeviceObject( pThis, i ), tsTag );
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
unsigned int daqBusDistributeMemory( register DAQ_BUS_T* pThis )
{
   //TODO!!!
   return 0;
}

/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusReset( register DAQ_BUS_T* pThis )
{
   DAQ_ASSERT( pThis != NULL );

   for( int i = daqBusGetFoundDevices( pThis )-1; i >= 0; i-- )
      daqDeviceReset( daqBusGetDeviceObject( pThis, i ) );
}

#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
/*! ---------------------------------------------------------------------------
 * @see daq.h
 */
void daqBusPrintInfo( register DAQ_BUS_T* pThis )
{
   unsigned int maxDevices = daqBusGetFoundDevices( pThis );
   for( unsigned int i = 0; i < maxDevices; i++ )
      daqDevicePrintInfo( daqBusGetDeviceObject( pThis, i ) );
}

#endif /* defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__) */

/*======================== DAQ- Descriptor functions ========================*/
#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
/*! --------------------------------------------------------------------------
 * @see daq_descriptor.h
 * @see daq.h
 */
void daqDescriptorPrintInfo( register DAQ_DESCRIPTOR_T* pThis )
{
   IMPLEMENT_CONVERT_BYTE_ENDIAN( uint32_t )

   mprintf( ESC_BOLD ESC_FG_CYAN
            "Device Descriptor:\n" ESC_NORMAL );
   mprintf( "  Slot:            %d\n", daqDescriptorGetSlot( pThis ) );
   mprintf( "  Channel:         %d\n", daqDescriptorGetChannel( pThis ) + 1 );
   mprintf( "  DIOB ID:         %d\n", daqDescriptorGetDiobId( pThis ) );
   mprintf( "  Post Mortem:     %s\n", daqDescriptorWasPM( pThis )?
                                       g_pYes : g_pNo );
   mprintf( "  High Resolution: %s\n", daqDescriptorWasHiRes( pThis )?
                                       g_pYes : g_pNo );
   mprintf( "  DAQ mode:        %s\n", daqDescriptorWasDaq( pThis )?
                                       g_pYes : g_pNo );
   mprintf( "  Trigger low:     0x%04x\n",
            daqDescriptorGetTriggerConditionLW( pThis ) );
   mprintf( "  Trigger high:    0x%04x\n",
            daqDescriptorGetTriggerConditionHW( pThis ) );
   mprintf( "  Trigger delay:   0x%04x\n",
            daqDescriptorGetTriggerDelay( pThis ) );
   mprintf( "  Seconds:       %08u\n",
            convertByteEndian_uint32_t( daqDescriptorGetTimeStampSec( pThis ) ));
   mprintf( "  Nanoseconds:   %09u\n",
            convertByteEndian_uint32_t( daqDescriptorGetTimeStampNanoSec( pThis )));
   mprintf( "  CRC:             0x%02x\n", daqDescriptorGetCRC( pThis ));
}

#endif // if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)

/*================================== EOF ====================================*/
