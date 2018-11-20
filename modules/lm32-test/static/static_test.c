/*!
 *
 * @brief     Testprogram for check whether local static variables
 *            within functions operates properly by the currently used
 *            compiler and bit field structures.
 *
 * @file      static_test.c
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author    Ulrich Becker <u.becker@gsi.de>
 * @date      20.11.2018
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
#include <stdbool.h>
#include "mini_sdb.h"
#include "eb_console_helper.h"
#include "helper_macros.h"

int foo( void )
{
   /*!
    * @bug local static variables (here: static int bar) will not initialized by zero! \n
    *      Initializing by all other values (except zero) works correct.  \n
    *      Perhaps this issue could be in the startup-code crt0.S resp. crt0.o.
    * @todo Fix this bug.
    *      As workaround the macro STATIC_LOCAL will solve this issue for the time being.
    */
   STATIC_LOCAL int bar = 0;
   bar++;
   return bar;
}

typedef struct
{
  uint16_t   a4:   4;
  uint16_t   b1:   1;
  uint16_t   c3:   3;
  uint16_t   d1:   1;
  uint16_t   e1:   1;
} BF_T;
STATIC_ASSERT( sizeof( BF_T ) == sizeof( uint16_t ) );

BF_T g_bf =
{
   .a4 = 3,
   .b1 = 0,
   .c3 = 7,
   .d1 = 0,
   .e1 = 1
};

void printBF( BF_T* pThis )
{
   mprintf( "a4 = %d\n", pThis->a4 );
   mprintf( "b1 = %d\n", pThis->b1 );
   mprintf( "c3 = %d\n", pThis->c3 );
   mprintf( "d1 = %d\n", pThis->d1 );
   mprintf( "e1 = %d\n", pThis->e1 );
   for( uint16_t m = 1 << 8 * sizeof(uint16_t) - 1; m != 0; m >>= 1 )
      mprintf( "%c", (m & *(uint16_t*)pThis)? '1' : '0' );
   mprintf( "\n" );
}

typedef struct
{
   int itemMember;
} CONTENT_T;

typedef struct
{
   int contMember;
   CONTENT_T item;
} CONTAINER_T;

CONTAINER_T container =
{
   .contMember = 42,
   .item =
   {
     .itemMember = 4711
   }
};

void printContainerFromCintent( CONTENT_T* pContent )
{
   mprintf( "Content = %d\n", CONTAINER_OF( pContent, CONTAINER_T, item )->contMember );
}



void main( void )
{
   discoverPeriphery();
   uart_init_hw();
   gotoxy( 0, 0 );
   clrscr();
   mprintf("Static-test compiler-version: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
   
   mprintf( "Foo: %d\n", foo() );
   mprintf( "Foo: %d\n", foo() );
   mprintf( "Foo: %d\n\n", foo() );
   
   printBF( &g_bf );
   
   BF_T bf = {0};
   bf.a4 = 10;
   bf.b1 = 1;
   bf.c3 = 6;
   bf.d1 = 0;
   bf.e1 = 1;
   printBF( &bf );
   
   bf.a4 = 15;
   bf.b1 = 0;
   bf.c3 = 5;
   bf.d1 = 1;
   bf.e1 = 0;
   printBF( &bf );

   printContainerFromCintent( &container.item );

}

//================================== EOF ======================================