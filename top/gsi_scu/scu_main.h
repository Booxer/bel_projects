/*!
 *  @file scu_main.h
 *  @brief Main module of SCU function generators in LM32.
 *
 *  @date 31.01.2020
 *  @copyright (C) 2019 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Ulrich Becker <u.becker@gsi.de>
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
#ifndef _SCU_MAIN_H
#define _SCU_MAIN_H

#if !defined(__lm32__) && !defined(__DOXYGEN__) && !defined(__DOCFSM__)
  #error This module is for the target LM32 only!
#endif
#ifndef MICO32_FULL_CONTEXT_SAVE_RESTORE
  #warning Macro MICO32_FULL_CONTEXT_SAVE_RESTORE is not defined in Makefile!
#endif

#include <stdint.h>
#include "syscon.h"
#include "eb_console_helper.h"
#include "scu_lm32_macros.h"
#include "irq.h"
#include "scu_bus.h"
#include "mini_sdb.h"
#include "board.h"
#include "uart.h"
#include "w1.h"
#include "scu_shared_mem.h"
#include "scu_mil.h"
#include "eca_queue_type.h"
#include "history.h"

/*!
 * @defgroup MIL_FSM Functions and macros which concerns the MIL-FSM
 */

/*!
 * @defgroup TASK Cooperative multitasking entry functions invoked by the scheduler.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_FG_PEDANTIC_CHECK
   /* CAUTION:
    * Assert-macros could be expensive in memory consuming and the
    * latency time can increase as well!
    * Especially in embedded systems with small resources.
    * Therefore use them for bug-fixing or developing purposes only!
    */
   #include <scu_assert.h>
   #define FG_ASSERT SCU_ASSERT
   #define FG_UNUSED
#else
   #define FG_ASSERT(__e)
   #define FG_UNUSED UNUSED
#endif

#define INTERVAL_1000MS 1000000000ULL
#define INTERVAL_2000MS 2000000000ULL
#define INTERVAL_100MS  100000000ULL
#define INTERVAL_84MS   84000000ULL
#define INTERVAL_10MS   10000000ULL
#define INTERVAL_200US  200000ULL
#define INTERVAL_150US  150000ULL
#define INTERVAL_100US  100000ULL
#define INTERVAL_10US   10000ULL
#define INTERVAL_5MS    5000000ULL
#define ALWAYS          0ULL

#define CLK_PERIOD (1000000 / USRCPUCLK) // USRCPUCLK in KHz
#define OFFS(SLOT) ((SLOT) * (1 << 16))

/*!
 * @see scu_shared_mem.h
 */
extern SCU_SHARED_DATA_T g_shared;


/*!
 * @brief Type of message origin
 */
typedef enum
{
   IRQ    = 0, /*!<@brief From interrupt              */
   SCUBUS = 1, /*!<@brief From non- MIL- device       */
   DEVBUS = 2, /*!<@brief From MIL-device.            */
   DEVSIO = 3, /*!<@brief From MIL-device via SCU-bus */
   SWI    = 4  /*!<@brief From Linux host             */
} MSG_T;

/*! ---------------------------------------------------------------------------
 * @ingroup TASK
 * @brief Declaration of the task type
 */
typedef struct _TASK_T
{
   const void*    pTaskData;  /*!<@brief Pointer to the memory-space of the current task */
   const uint64_t interval;   /*!<@brief interval of the task */
   uint64_t       lasttick;   /*!<@brief when was the task ran last */
   void (*func)(struct _TASK_T*); /*!<@brief pointer to the function of the task */
} TASK_T;


/*!
 * @brief Message size of message queue.
 */
#define QUEUE_CNT 5

/*! ---------------------------------------------------------------------------
 * @brief enables msi generation for the specified channel. \n
 * Messages from the scu bus are send to the msi queue of this cpu with the offset 0x0. \n
 * Messages from the MIL extension are send to the msi queue of this cpu with the offset 0x20. \n
 * A hardware macro is used, which generates msis from legacy interrupts. \n
 * @param channel number of the channel between 0 and MAX_FG_CHANNELS-1
 * @see disable_slave_irq
 */
void enable_scub_msis( const unsigned int channel );

/*! ---------------------------------------------------------------------------
 * @brief Scans for function generators on mil extension and scu bus.
 */
void scanFgs( void );

/*! ---------------------------------------------------------------------------
 * @brief helper function which clears the state of a dev bus after malfunction
 */
void clear_handler_state( const uint8_t socket );


//#define CONFIG_DEBUG_FG_SIGNAL
/*! ---------------------------------------------------------------------------
 * @brief Send a signal back to the Linux-host (SAFTLIB)
 * @param sig Signal
 * @param channel Concerning channel number.
 */
STATIC inline void sendSignal( const SIGNAL_T sig, const unsigned int channel )
{
   *(volatile uint32_t*)(char*)
   (pCpuMsiBox + g_shared.fg_regs[channel].mbx_slot * sizeof(uint16_t)) = sig;
   hist_addx( HISTORY_XYZ_MODULE, signal2String( sig ), channel );
#ifdef CONFIG_DEBUG_FG_SIGNAL
   #warning CONFIG_DEBUG_FG_SIGNAL is defined this will destroy the timing!
   mprintf( ESC_DEBUG"Signal: %s, channel: %d sent\n"ESC_NORMAL,
            signal2String( sig ), channel );
#endif
}

/*! ---------------------------------------------------------------------------
 * @brief Returns the index number of a FG-macro in the FG-list by the
 *        channel number
 */
STATIC inline
int getFgMacroIndexFromFgRegister( const unsigned int channel )
{
   FG_ASSERT( channel < ARRAY_SIZE( g_shared.fg_regs ) );
   return g_shared.fg_regs[channel].macro_number;
}

/*! ---------------------------------------------------------------------------
 * @brief Returns the Function Generator macro of the given channel.
 */
STATIC inline FG_MACRO_T getFgMacroViaFgRegister( const unsigned int channel )
{
   FG_ASSERT( getFgMacroIndexFromFgRegister( channel ) >= 0 );
   FG_ASSERT( getFgMacroIndexFromFgRegister( channel ) < ARRAY_SIZE( g_shared.fg_macros ));
   return g_shared.fg_macros[getFgMacroIndexFromFgRegister( channel )];
}

/*! ---------------------------------------------------------------------------
 * @brief Returns "true" if the function generator of the given channel
 *        present.
 * @see FOR_EACH_FG
 * @see FOR_EACH_FG_CONTINUING
 */
STATIC inline bool isFgPresent( const unsigned int channel )
{
   if( channel >= MAX_FG_CHANNELS )
      return false;
   if( getFgMacroIndexFromFgRegister( channel ) < 0 )
      return false;
   return getFgMacroViaFgRegister( channel ).outputBits != 0;
}

/*! ---------------------------------------------------------------------------
 * @brief Returns the socked number of the given channel.
 * @note The lower 4 bits of the socket number contains the slot-number
 *       of the SCU-bus which can masked out by SCU_BUS_SLOT_MASK.
 */
STATIC inline uint8_t getSocket( const unsigned int channel )
{
   FG_ASSERT( isFgPresent( channel ) );
   return getFgMacroViaFgRegister( channel ).socket;
}

/*! ---------------------------------------------------------------------------
 * @brief Returns the device number of the given channel.
 */
STATIC inline uint8_t getDevice( const unsigned int channel )
{
   FG_ASSERT( isFgPresent( channel ) );
   return getFgMacroViaFgRegister( channel ).device;
}

#ifdef __cplusplus
}
#endif
#endif /* _SCU_MAIN_H */
/* ================================= EOF ====================================*/

