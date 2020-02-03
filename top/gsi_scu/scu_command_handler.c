/*!
 * @file scu_command_handler.c Module for receiving of commands from SAFT-LIB
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author Ulrich Becker <u.becker@gsi.de>
 * @date 03.02.2020
 * Outsourced from scu_main.c
 */

#include "scu_command_handler.h"

extern FG_MESSAGE_BUFFER_T    g_aMsg_buf[QUEUE_CNT];
extern FG_CHANNEL_T           g_aFgChannels[MAX_FG_CHANNELS];
extern volatile uint16_t*     g_pScub_base;
extern volatile unsigned int* g_pScu_mil_base;

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
void sw_irq_handler( register TASK_T* pThis FG_UNUSED )
{
   FG_ASSERT( pThis->pTaskData == NULL );

   if( !has_msg( &g_aMsg_buf[0], SWI ) )
      return; /* Nothing to do.. */

#ifdef CONFIG_READ_MIL_TIME_GAP
   if( !isMilFsmInST_WAIT() )
      return;
#endif

   const MSI_T m = remove_msg( &g_aMsg_buf[0], SWI );
   if( m.adr != 0x10 ) //TODO From where the fuck comes 0x10!!!
      return;

   const unsigned int code  = m.msg >> BIT_SIZEOF( uint16_t );
   const unsigned int value = m.msg & 0xffff;
   printSwIrqCode( code, value );

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
                       g_pScub_base,
                       g_pScu_mil_base );
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
      #ifdef CONFIG_READ_MIL_TIME_GAP
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
