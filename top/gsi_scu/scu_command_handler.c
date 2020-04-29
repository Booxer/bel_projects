/*!
 * @file scu_command_handler.c
 * @brief  Module for receiving of commands from SAFT-LIB
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author Ulrich Becker <u.becker@gsi.de>
 * @date 03.02.2020
 * Outsourced from scu_main.c
 */
#include <scu_fg_macros.h>
#ifdef CONFIG_MIL_FG
  #include <scu_mil_fg_handler.h>
#endif
#include <scu_command_handler.h>
#ifdef CONFIG_SCU_DAQ_INTEGRATION
  #include <daq_main.h>
  #include <daq_command_interface_uc.h>
#endif

extern FG_MESSAGE_BUFFER_T    g_aMsg_buf[QUEUE_CNT];
extern FG_CHANNEL_T           g_aFgChannels[MAX_FG_CHANNELS];
extern volatile uint16_t*     g_pScub_base;
#ifdef CONFIG_MIL_FG
extern volatile unsigned int* g_pScu_mil_base;
#endif
#ifdef DEBUG_SAFTLIB
  #warning "DEBUG_SAFTLIB is defined! This could lead to timing problems!"
#endif

//#define CONFIG_DEBUG_SWI

#ifdef CONFIG_DEBUG_SWI
#warning Function printSwIrqCode() is activated! In this mode the software will not work!
/*! ---------------------------------------------------------------------------
 * @brief For debug purposes only!
 */
STATIC void printSwIrqCode( const unsigned int code, const unsigned int value )
{
   const char* str;
   #define _SWI_CASE_ITEM( i ) case i: str = #i; break
   switch( code )
   {
      _SWI_CASE_ITEM( FG_OP_INITIALIZE );
      _SWI_CASE_ITEM( FG_OP_RFU );
      _SWI_CASE_ITEM( FG_OP_CONFIGURE );
      _SWI_CASE_ITEM( FG_OP_DISABLE_CHANNEL );
      _SWI_CASE_ITEM( FG_OP_RESCAN );
      _SWI_CASE_ITEM( FG_OP_CLEAR_HANDLER_STATE );
      _SWI_CASE_ITEM( FG_OP_PRINT_HISTORY );
      default: str = "unknown"; break;
   }
   #undef _SWI_CASE_ITEM
   mprintf( ESC_DEBUG"SW-IRQ: %s\tValue: %d"ESC_NORMAL"\n", str, value );
}
#else
#define printSwIrqCode( code, value )
#endif

/*! ---------------------------------------------------------------------------
 * @ingroup TASK
 * @brief Software irq handler
 *
 * dispatch the calls from linux to the helper functions
 * called via scheduler in main loop
 * @param pThis pointer to the current task object
 * @see schedule
 */
void commandHandler( register TASK_T* pThis FG_UNUSED )
{
   FG_ASSERT( pThis->pTaskData == NULL );

#ifdef CONFIG_SCU_DAQ_INTEGRATION
   /*!
    * Executing a possible ADDAC-DAQ command if requested...
    */
   executeIfRequested( &g_scuDaqAdmin );
#endif


#if defined( CONFIG_MIL_FG ) && defined( CONFIG_READ_MIL_TIME_GAP )
   if( !isMilFsmInST_WAIT() )
   { /*
      * Wait a round...
      */
      return;
   }
#endif

   MSI_T m;
   /*
    * Is a message from SATF-LIB for FG present?
    */
   if( !getMessageSave( &m, &g_aMsg_buf[0], SWI ) )
   { /*
      * No!
      */
      return;
   }

   FG_ASSERT( m.adr == ADDR_SWI );

   const unsigned int code  = m.msg >> BIT_SIZEOF( uint16_t );
   const unsigned int value = m.msg & 0xFFFF;
   printSwIrqCode( code, value );

   /*!
    * Verifying the command parameter for all commands with a parameter.
    */
   switch( code )
   {
      case FG_OP_INITIALIZE:          /* Go immediately to next case. */
      case FG_OP_CONFIGURE:           /* Go immediately to next case. */
      case FG_OP_DISABLE_CHANNEL:     /* Go immediately to next case. */
      case FG_OP_CLEAR_HANDLER_STATE:
      {
         if( value < ARRAY_SIZE( g_aFgChannels ) )
            break;

         mprintf( ESC_ERROR"Value %d out of range!"ESC_NORMAL"\n", value );
         return;
      }
      default: break;
   }

   /*
    * Executing the SAFT-LIB command if known.
    */
   switch( code )
   {
      case FG_OP_INITIALIZE:
      {
         hist_addx(HISTORY_XYZ_MODULE, "init_buffers", m.msg);
        #if __GNUC__ >= 9
         #pragma GCC diagnostic push
         #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
        #endif
         init_buffers( &g_shared.fg_regs[0],
                       m.msg,
                       &g_shared.fg_macros[0],
                       g_pScub_base
                     #ifdef CONFIG_MIL_FG
                       , g_pScu_mil_base
                     #endif
                     );
        #if __GNUC__ >= 9
         #pragma GCC diagnostic pop
        #endif
         g_aFgChannels[value].param_sent = 0;
         break;
      }

      case FG_OP_RFU:
      {
         break;
      }

      case FG_OP_CONFIGURE:
      {
      #if defined( CONFIG_MIL_FG ) && defined( CONFIG_READ_MIL_TIME_GAP )
         suspendGapReading(); // TEST!!!
      #endif
         enable_scub_msis( value );
         configure_fg_macro( value );
      #ifdef DEBUG_SAFTLIB
         mprintf( "+%d ", value );
      #endif
         break;
      }

      case FG_OP_DISABLE_CHANNEL:
      {
         disable_channel( value );
      #ifdef DEBUG_SAFTLIB
         mprintf( "-%d ", value );
      #endif
         break;
      }

      case FG_OP_RESCAN:
      { //rescan for fg macros
         scanFgs();
         break;
      }

      case FG_OP_CLEAR_HANDLER_STATE:
      {
         clear_handler_state(value);
         break;
      }

      case FG_OP_PRINT_HISTORY:
      {
       #ifdef HISTORY
         hist_print(1);
       #else
         mprintf( "No history!\n" );
       #endif
         break;
      }

      default:
      {
         mprintf("swi: 0x%x\n", m.adr);
         mprintf("     0x%x\n", m.msg);
         break;
      }
   }
#ifdef CONFIG_DEBUG_FG
   #warning When CONFIG_DEBUG_FG defined then the timing will destroy!
   mprintf( ESC_FG_CYAN ESC_BOLD"FG-command: %s: %d\n"ESC_NORMAL,
            fgCommand2String( code ), value );
#endif
}

/*================================== EOF ====================================*/