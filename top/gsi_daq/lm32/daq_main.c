/*!
 *  @file daq_main.c
 *  @brief Main module for daq_control (including main())
 *
 *  @date 27.02.2019
 *  @copyright (C) 2019 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Ulrich Becker <u.becker@gsi.de>
 *
 ******************************************************************************
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */
#include <daq_command_interface_uc.h>
#include <daq_main.h>
#include <mini_sdb.h>
#include <eb_console_helper.h>
#include <dbg.h>

#ifndef CONFIG_DAQ_SINGLE_APP
extern volatile uint16_t* g_pScub_base;
#endif

#ifdef CONFIG_DAQ_SINGLE_APP
static
#endif
DAQ_ADMIN_T g_scuDaqAdmin;

static inline uint32_t getInterruptPending( void )
{
   uint32_t ip;
   asm volatile ("rcsr %0, ip": "=r"(ip));
   return ip;
}


/*! ---------------------------------------------------------------------------
 */
int daqScanScuBus( DAQ_BUS_T* pDaqDevices )
{
#ifdef CONFIG_DAQ_SINGLE_APP
   void* pScuBusBase = find_device_adr( GSI, SCU_BUS_MASTER );

   /* That's not fine, but it's not my idea. */
   if( pScuBusBase == (void*)ERROR_NOT_FOUND )
   {
      DBPRINT1( ESC_BOLD ESC_FG_RED
               "ERROR: find_device_adr() didn't find it!\n"
               ESC_NORMAL );
      return DAQ_RET_ERR_DEVICE_ADDRESS_NOT_FOUND;
   }
   int ret = daqBusFindAndInitializeAll( pDaqDevices, pScuBusBase );
#else
   int ret = daqBusFindAndInitializeAll( pDaqDevices, (void*)g_pScub_base );
#endif
   if( ret < 0 )
   {
      DBPRINT1(  ESC_BOLD ESC_FG_RED
                 "ERROR: in daqBusFindAndInitializeAll()\n"
                 ESC_NORMAL );
      return DAQ_RET_ERR_DEVICE_ADDRESS_NOT_FOUND;
   }
#ifdef DEBUGLEVEL
   if( ret == 0 )
      DBPRINT1( "WARNING: No DAQ devices present!\n" );
   else
   {
      DBPRINT1( "DBG: %d DAQ devices found.\n", ret );
      DBPRINT1( "DBG: Total number of all used channels: %d\n",
                daqBusGetUsedChannels( pDaqDevices ) );
   }
#endif
   return DAQ_RET_OK;
}

/*! ---------------------------------------------------------------------------
 */
static inline void handleContinuousMode( DAQ_CANNEL_T* pChannel )
{
   if( !daqChannelTestAndClearDaqIntPending( pChannel ) )
      return;
#ifdef CONFIG_DAQ_SW_SEQUENCE
   pChannel->sequenceContinuous++;
#endif
   if( daqChannelGetDaqFifoWords( pChannel ) == 0 )
   {
      DBPRINT1( ESC_BOLD ESC_FG_RED
                "DBG: WARNING: Discarding continuous block: "
                      "Slot: %d, Channel: %d !\n"
                ESC_NORMAL,
                daqChannelGetSlot( pChannel ),
                daqChannelGetNumber( pChannel )
              );
      return;
   }

   ramPushDaqDataBlock( &g_scuDaqAdmin.oRam, pChannel, true );
   daqChannelDecrementBlockCounter( pChannel );
}


/*! ---------------------------------------------------------------------------
 */
static inline bool forEachContinuousCahnnel( DAQ_DEVICE_T* pDevice )
{
   for( unsigned int channelNr = 0;
        channelNr < daqDeviceGetMaxChannels( pDevice ); channelNr++ )
   {
      if( executeIfRequested( &g_scuDaqAdmin ) )
         return true;
      handleContinuousMode( daqDeviceGetChannelObject( pDevice, channelNr ) );
   }
   return false;
}

/*! ---------------------------------------------------------------------------
 */
static inline void handleHiresMode( DAQ_CANNEL_T* pChannel )
{
   if( !daqChannelTestAndClearHiResIntPending( pChannel ) )
      return;

   daqChannelDisableHighResolution( pChannel );
#ifdef CONFIG_DAQ_SW_SEQUENCE
   pChannel->sequencePmHires++;
#endif
   ramPushDaqDataBlock( &g_scuDaqAdmin.oRam, pChannel, false );
}

/*! ---------------------------------------------------------------------------
 */
static inline bool forEachHiresChannel( DAQ_DEVICE_T* pDevice )
{
   for( unsigned int channelNr = 0;
        channelNr < daqDeviceGetMaxChannels( pDevice ); channelNr++ )
   {
      //if( executeIfRequested( &g_DaqAdmin ) )
      //   return true;
      handleHiresMode( daqDeviceGetChannelObject( pDevice, channelNr ) );
   }
   return false;
}

/*! ---------------------------------------------------------------------------
 */
static inline void handlePostMortemMode( DAQ_CANNEL_T* pChannel )
{
   if( !pChannel->properties.postMortemEvent )
      return;
   if( !daqChannelIsPmHiResFiFoFull( pChannel ) )
      return;

   pChannel->properties.postMortemEvent = false;
   daqChannelDisablePostMortem( pChannel ); //!!
#ifdef CONFIG_DAQ_SW_SEQUENCE
   pChannel->sequencePmHires++;
#endif
   ramPushDaqDataBlock( &g_scuDaqAdmin.oRam, pChannel, false );
}

/*! ---------------------------------------------------------------------------
 */
static inline bool forEachPostMortemChennel( DAQ_DEVICE_T* pDevice )
{
   for( unsigned int channelNr = 0;
        channelNr < daqDeviceGetMaxChannels( pDevice ); channelNr++ )
   {
      if( executeIfRequested( &g_scuDaqAdmin ) )
         return true;
      handlePostMortemMode( daqDeviceGetChannelObject( pDevice, channelNr ) );
   }
   return false;
}

/*! ---------------------------------------------------------------------------
 */
#ifdef CONFIG_DAQ_SINGLE_APP
static inline
#endif
void forEachScuDaqDevice( void )
{
   bool isIrq;

   // TODO disable irq
 //!!  isIrq = g_DaqAdmin.isIrq;
 //!!  g_DaqAdmin.isIrq = false;
   // TODO enable irq

   isIrq = true; //!!

   uint32_t pending = getInterruptPending();
   if( pending != 0 )
      DBPRINT1( "DBG: pending: 0x%08x\n", pending );

   for( unsigned int deviceNr = 0;
       deviceNr < daqBusGetFoundDevices( &g_scuDaqAdmin.oDaqDevs ); deviceNr++ )
   {
      DAQ_DEVICE_T* pDevice = daqBusGetDeviceObject( &g_scuDaqAdmin.oDaqDevs,
                                                                    deviceNr );
      if( isIrq )
      {
         DAQ_REGISTER_T* volatile pIntFlags =
                                         daqDeviceGetInterruptFlags( pDevice );
         if( _daqDeviceTestAndClearDaqInt( pIntFlags ) )
         {
            if( forEachContinuousCahnnel( pDevice ))
            {
               DBPRINT1( "DBG: Leaving loop 1\n" );
               return;
            }
         }
         if( _daqDeviceTestAndClearHiResInt( pIntFlags ) )
         {
            if( forEachHiresChannel( pDevice ) )
            {
               DBPRINT1( "DBG: Leaving loop 2\n" );
               return;
            }
         }
      }

      if( forEachPostMortemChennel( pDevice ) )
      {
         DBPRINT1( "DBG: Leaving loop 3\n" );
         return;
      }
   }
   executeIfRequested( &g_scuDaqAdmin );
}

/*================================= main ====================================*/
#ifdef CONFIG_DAQ_SINGLE_APP
extern uint32_t _endram;
#define STACK_MAGIC 0xAAAAAAAA

/*! ---------------------------------------------------------------------------
 */
int main( void )
{
   _endram = STACK_MAGIC;
#ifdef DEBUGLEVEL
   discoverPeriphery();
   uart_init_hw();
   gotoxy( 0, 0 );
   clrscr();
   DBPRINT1( "DAQ control started; Compiler: "COMPILER_VERSION_STRING"\n" );
   DBPRINT1( "DAQ End of RAM:  0x%08x [0x%08x]\n", &_endram, _endram );
#endif

   scuDaqInitialize( &g_scuDaqAdmin );

   while( true )
   {
      DAQ_ASSERT( _endram == STACK_MAGIC );
      forEachScuDaqDevice();
   }
   return 0;
}
#endif /* CONFIG_DAQ_SINGLE_APP */
/*================================== EOF ====================================*/