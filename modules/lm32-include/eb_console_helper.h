/*!
 *
 * @brief     Definition of some terminal escape sequences (ISO 6429)
 *
 *            Helpful for outputs via eb-console
 *
 * @file      eb_console_helper.h
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author    Ulrich Becker <u.becker@gsi.de>
 * @date      12.11.2018
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
#ifndef _EB_CONSOLE_HELPER_H
#define _EB_CONSOLE_HELPER_H

#if defined(__lm32__)
  #include <mprintf.h>
#elif !defined( mprintf )
  #include <stdio.h>
  #define mprintf printf
#endif

#define ESC_FG_BLACK   "\e[30m" /*!< @brief Foreground color black   */
#define ESC_FG_RED     "\e[31m" /*!< @brief Foreground color red     */
#define ESC_FG_GREEN   "\e[32m" /*!< @brief Foreground color green   */
#define ESC_FG_YELLOW  "\e[33m" /*!< @brief Foreground color yellow  */
#define ESC_FG_BLUE    "\e[34m" /*!< @brief Foreground color blue    */
#define ESC_FG_MAGNETA "\e[35m" /*!< @brief Foreground color magneta */
#define ESC_FG_CYAN    "\e[36m" /*!< @brief Foreground color cyan    */
#define ESC_FG_WHITE   "\e[37m" /*!< @brief Foreground color white   */

#define ESC_BG_BLACK   "\e[40m" /*!< @brief Background color black   */
#define ESC_BG_RED     "\e[41m" /*!< @brief Background color red     */
#define ESC_BG_GREEN   "\e[42m" /*!< @brief Background color green   */
#define ESC_BG_YELLOW  "\e[43m" /*!< @brief Background color yellow  */
#define ESC_BG_BLUE    "\e[44m" /*!< @brief Background color blue    */
#define ESC_BG_MAGNETA "\e[45m" /*!< @brief Background color magneta */
#define ESC_BG_CYAN    "\e[46m" /*!< @brief Background color cyan    */
#define ESC_BG_WHITE   "\e[47m" /*!< @brief Background color white   */

#define ESC_BOLD      "\e[1m"   /*!< @brief Bold on  */
#define ESC_BLINK     "\e[5m"   /*!< @brief Blink on */
#define ESC_NORMAL    "\e[0m"   /*!< @brief All attributes off */
#define ESC_HIDDEN    "\e[8m"   /*!< @brief Hidden on */

#ifdef __cplusplus
extern "C" {
#endif

   
/*!
 * @brief Set cursor position.
 * @param x Column position (horizontal)
 * @param y Line position (vertical)
 */
static inline void gotoxy( int x, int y )
{
   mprintf( "\e[%d;%dH", y, x );
}

/*!
 * @brief Clears the entire console screen and moves the cursor to (0,0)
 */
static inline void clrscr( void )
{
   mprintf( "\e[2J" );
}

/*!
 * @brief Clears all characters from the cursor position to the end of the
 *        line (including the character at the cursor position).
 */
static inline void clrline( void )
{
   mprintf( "\e[K" );
}

#ifdef __cplusplus
}
#endif


#endif /* ifndef _EB_CONSOLE_HELPER_H */
/*================================== EOF ====================================*/
