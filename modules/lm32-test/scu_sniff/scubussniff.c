/*!
 *  @file scubussniff.c
 *  @brief Sniffs for all connected slaves on SCU-bus.
 *  @date 15.11.2018
 *  @copyright (C) 2018 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Ulrich Becker <u.becker@gsi.de>
 *
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT AqNY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#include "mini_sdb.h"
#include "scu_bus.h"
#include "eb_console_helper.h"
#include "lm32_hexdump.h"
#include "dbg.h"

#define LINE_OFFSET 5

void _segfault(int sig)
{
   mprintf( ESC_FG_RED ESC_BOLD "Segmentation fault: %d\n" ESC_NORMAL, sig );
   while( 1 );
}

void main( void )
{
   SCUBUS_SLAVE_FLAGS_T slaveFlags;
   void* pScuBusBase;

   discoverPeriphery();
   uart_init_hw();

   gotoxy( 0, 0 );
   clrscr();
   mprintf( ESC_BOLD "SCU-Bus Sniffer\n" ESC_NORMAL );

   pScuBusBase = find_device_adr( GSI, SCU_BUS_MASTER );
   if( pScuBusBase == (void*)ERROR_NOT_FOUND )
   {
      mprintf( ESC_BOLD ESC_FG_RED "ERROR: Couldn't found address of SCU_BUS_MASTER!\n" ESC_NORMAL );
      return;
   }

   mprintf( "Base address: 0x%08x\n", pScuBusBase );



   slaveFlags = scuBusFindAllSlaves( pScuBusBase );
   mprintf( "%d Slaves found\n", scuBusGetNumberOfSlaves( slaveFlags ) );

   int i = LINE_OFFSET;
   //while( uart_read_byte() != 'c' );
   for( int slot = SCUBUS_START_SLOT; slot <= MAX_SCU_SLAVES; slot++ )
   {
      if( !scuBusIsSlavePresent( slaveFlags, slot ) )
         continue;

      const void* pSlave = scuBusGetAbsSlaveAddr( pScuBusBase, slot );
      hexdump( pSlave, 1024 );
      //mprintf( "%s\n", pSlave + SLAVE_INFO_TEXT );
      gotoxy( 0, i );
      mprintf( "Slot %d:", slot );
      gotoxy( 10, i );
      mprintf( "Address 0x%08x,", pSlave );
      gotoxy( 30, i );
      mprintf( "CID_SYS %d,", scuBusGetSlaveValue16( pSlave, CID_SYS ) );
      gotoxy( 42, i );
      mprintf( "CID_GROUP %d,",scuBusGetSlaveValue16( pSlave, CID_GROUP ) );
      gotoxy( 56, i );
      mprintf( "SLAVE_VERSION %d,", scuBusGetSlaveValue16( pSlave, SLAVE_VERSION ) );
      i++;
   }
}

/*================================== EOF ====================================*/