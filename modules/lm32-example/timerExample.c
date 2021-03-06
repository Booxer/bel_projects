/********************************************************************************************
 *  timerExample.c
 *
 *  created : 2020
 *  author  : Dietrich Beck, some code is pirated from Stefan Rauch, GSI-Darmstadt
 *  version : 26-May-2020
 *
 *  very basic example program for using a timer via IRQ and callback in an lm32 softcore
 *
 *  it also demonstrated how to read the uptime of the user lm32 'ftm cluter'
 * 
 * -------------------------------------------------------------------------------------------
 * License Agreement for this software:
 *
 * Copyright (C) 2017  Dietrich Beck
 * GSI Helmholtzzentrum für Schwerionenforschung GmbH
 * Planckstraße 1
 * D-64291 Darmstadt
 * Germany
 *
 * Contact: d.beck@gsi.de
 *
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
 *
 * For all questions and ideas contact: d.beck@gsi.de
 * Last update: 25-April-2015
 ********************************************************************************************/

// standard includes 
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

// includes specific for bel_projects 
#include "pp-printf.h"                                    // print function; please use 'pp_printf' instead of 'printf'
#include "mini_sdb.h"                                     // required for using Wisbhone self-describing bus from lm32

#include "aux.h"                                          // basic helper routines for the lm32 CPU
#include "uart.h"                                         // WR console
#include "../wb_timer/wb_timer_regs.h"                    // WB timer

// shared memory map for communication via Wishbone 
#include "timerExample_shared_mmap.h"

// stuff required for lm32 environment 
extern uint32_t*       _startshared[];
unsigned int cpuId, cpuQty;
#define SHARED __attribute__((section(".shared")))
uint64_t SHARED dummy = 0;

// variables for this program
volatile uint32_t *wb_timer_preset;                       // preset register of timer
volatile uint32_t *wb_timer_config;                       // config register of time
volatile uint32_t *wb_timer_counter;                      // counter of timer
volatile uint32_t *wb_timer_ticklen;                      // period of a counter tick

// generic init used by lm32 programs
void init()
{
  discoverPeriphery();                                    // mini-sdb: get info on important Wishbone infrastructure
  uart_init_hw();                                         // init UART, required for pp_printf...
  cpuId = getCpuIdx();                                    // get ID of THIS CPU 
} // init

// implement simple callback routine for our timer; this demonstrates
// - how to set up a callback function for the timer IRQ
// - how to read and calculate the uptime of the lm32 ftm-cluster
// as a bonus, we calculate the approximate delay penalty when receiving an IRQ via a callback function
void timer_handler() {
  static uint32_t len    = 0x0;
  static uint32_t preset = 0x0;

  uint64_t ts;
  uint32_t irqDelay;

  if (!len)    len    = *wb_timer_ticklen;                // read tick length [ns] of counter upon first run
  if (!preset) preset = *wb_timer_preset;                 // read timer preset [ticks]

  irqDelay = (preset - *wb_timer_counter) * len;          // read actual counter value, calculate delay for IRQ and convert to nanoseconds

  pp_printf("timer_handler: ftm uptime %lu seconds, IRQ delay %lu [ns]\n", (uint32_t)(getCpuTime() / 1000000000UL), irqDelay);
} // timer_handler

// init IRQ table; here we just configure the timer
void init_irq_table() {
  isr_table_clr();                                        // clear table
  // isr_ptr_table[0] = &irq_handler;                     // 0: hard-wired MSI; don't use here
  isr_ptr_table[1] = &timer_handler;                      // 1: hard-wired timer
  irq_set_mask(0x02);                                     // only use timer
  irq_enable();                                           // enable IRQs
  pp_printf("IRQ table configured.\n");
} // init_irq_table

// main loop
int main(void) {

  init();                                                 // basic init for a lm32 program

  pp_printf("Hello World!\n");

  if ((uint32_t)pCpuWbTimer != ERROR_NOT_FOUND) {         // defined in mini_sdb.h
    
    // calculate register addresses for timer
    wb_timer_config        = pCpuWbTimer + (WB_TIMER_CONFIG >> 2);
    wb_timer_preset        = pCpuWbTimer + (WB_TIMER_PRESET >> 2);
    wb_timer_counter       = pCpuWbTimer + (WB_TIMER_COUNTER >> 2);
    wb_timer_ticklen       = pCpuWbTimer + (WB_TIMER_TICKLEN >> 2);
    
    // set timer to 1 second
    *wb_timer_preset = 1000000000UL / *wb_timer_ticklen;  
    
    init_irq_table();                                     // init IRQ
    *wb_timer_config = 0x1;                               // start timer
  } // if pCpuWbTimer
  else pp_printf("lm32 timer not found!\n");

  pp_printf("timer started\n");

  while (1) {
    uwait(100000);                                        // uwait() is an alternative to usleep(); uwait() uses the timer of the lm32 user cluster
  } // while
} /* main */
