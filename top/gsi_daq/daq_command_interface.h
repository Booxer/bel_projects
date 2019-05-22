/*!
 *  @file daq_command_interface.h
 *  @brief Definition of DAQ-commandos and data object for shared memory
 *
 *  @note This file is suitable for LM32-apps within the SCU environment and
 *        for Linux applications. \n
 *        CAUTION:
 *        The LM32 application has to be compiled at first so that
 *        by the Makefile generated file <generated/shared_mmap.h>
 *        is present when the Lunux part becomes compiled.
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
#ifndef _DAQ_COMMAND_INTERFACE_H
#define _DAQ_COMMAND_INTERFACE_H

#include <generated/shared_mmap.h>
#include <scu_bus_defines.h>
#include <daq_ramBuffer.h>
#include <stdint.h>

/*!
 * @ingroup DAQ
 * @defgroup DAQ_INTERFACE
 * @brief DAQ communication module between Linux and LM32
 * @{
 */

/*!
 * @brief Magic number of DAQ application. Useful in recognizing
 *        the LM32 DAQ application in the shared memory.
 */
#define DAQ_MAGIC_NUMBER           ((uint32_t)0xCAFEAD04)

/*!
 * @defgroup DAQ_STATUS_MESSAGES
 * @brief Status and error messages of the DAQ LM32 application.
 * @{
 */

/*!
 * @brief Return code type of the DAQ LM32 application.
 */
typedef int32_t DAQ_RETURN_CODE_T;


#define DAQ_RET_ERR_UNKNOWN_OPERATION        -1
#define DAQ_RET_ERR_SLAVE_NOT_PRESENT        -2
#define DAQ_RET_ERR_CHANNEL_NOT_PRESENT      -3
#define DAQ_RET_ERR_DEVICE_ADDRESS_NOT_FOUND -4
#define DAQ_RET_ERR_CHANNEL_OUT_OF_RANGE     -5
#define DAQ_RET_ERR_SLAVE_OUT_OF_RANGE       -6
#define DAQ_RET_ERR_WRONG_SAMPLE_PARAMETER   -7

#define DAQ_RET_OK                            0
#define DAQ_RET_RESCAN                        1

/*! @} DAQ_STATUS_MESSAGES */

#ifndef DAQ_OP_OFFSET
  #define DAQ_OP_OFFSET 0
#endif

/*!
 * @brief Operation code to controlling the DAQs from host.
 */
typedef enum
{
   DAQ_OP_IDLE                    = 0,
   DAQ_OP_LOCK                    = DAQ_OP_OFFSET +  1,
   DAQ_OP_GET_ERROR_STATUS        = DAQ_OP_OFFSET +  2,
   DAQ_OP_RESET                   = DAQ_OP_OFFSET +  3,
   DAQ_OP_GET_MACRO_VERSION       = DAQ_OP_OFFSET +  4,
   DAQ_OP_GET_SLOTS               = DAQ_OP_OFFSET +  5,
   DAQ_OP_GET_CHANNELS            = DAQ_OP_OFFSET +  6,
   DAQ_OP_RESCAN                  = DAQ_OP_OFFSET +  7,
   DAQ_OP_PM_ON                   = DAQ_OP_OFFSET +  8,
   DAQ_OP_HIRES_ON                = DAQ_OP_OFFSET +  9,
   DAQ_OP_PM_HIRES_OFF            = DAQ_OP_OFFSET + 10,
   DAQ_OP_CONTINUE_ON             = DAQ_OP_OFFSET + 11,
   DAQ_OP_CONTINUE_OFF            = DAQ_OP_OFFSET + 12,
   DAQ_OP_SET_TRIGGER_CONDITION   = DAQ_OP_OFFSET + 13,
   DAQ_OP_GET_TRIGGER_CONDITION   = DAQ_OP_OFFSET + 14,
   DAQ_OP_SET_TRIGGER_DELAY       = DAQ_OP_OFFSET + 15,
   DAQ_OP_GET_TRIGGER_DELAY       = DAQ_OP_OFFSET + 16,
   DAQ_OP_SET_TRIGGER_MODE        = DAQ_OP_OFFSET + 17,
   DAQ_OP_GET_TRIGGER_MODE        = DAQ_OP_OFFSET + 18,
   DAQ_OP_SET_TRIGGER_SOURCE_CON  = DAQ_OP_OFFSET + 19,
   DAQ_OP_GET_TRIGGER_SOURCE_CON  = DAQ_OP_OFFSET + 20,
   DAQ_OP_SET_TRIGGER_SOURCE_HIR  = DAQ_OP_OFFSET + 21,
   DAQ_OP_GET_TRIGGER_SOURCE_HIR  = DAQ_OP_OFFSET + 22
} DAQ_OPERATION_CODE_T;
#ifndef __DOXYGEN__
STATIC_ASSERT( sizeof( DAQ_OPERATION_CODE_T ) == sizeof(uint32_t) );
#endif

/*!
 * @brief Sub operation code of the sample rate for the DAQ continuous mode.
 * @see DAQ_OP_CONTINUE_ON
 */
typedef enum
{
   DAQ_SAMPLE_1MS   = 1, /*!<@brief Sample rate 1 millisecond.    */
   DAQ_SAMPLE_100US = 2, /*!<@brief Sample rate 100 microseconds. */
   DAQ_SAMPLE_10US  = 3  /*!<@brief Sample rate 10 microseconds.  */
} DAQ_SAMPLE_RATE_T;

/*!
 * @brief Data type for selecting a DAQ residing in a SCU slot
 *        and one of its channel.
 */
typedef struct PACKED_SIZE
{
   uint16_t  deviceNumber;
   uint16_t  channel;
} DAQ_CHANNEL_LOCATION_T;
#ifndef __DOXYGEN__
STATIC_ASSERT( sizeof( DAQ_CHANNEL_LOCATION_T ) == 2 * sizeof(uint16_t));
#endif

/*!
 * @brief Data type for locating a DAQ device and channel plus optional
 *        parameter list of maximum four parameters.
 */
typedef struct PACKED_SIZE
{
   DAQ_CHANNEL_LOCATION_T location;
   DAQ_REGISTER_T         param1;
   DAQ_REGISTER_T         param2;
   DAQ_REGISTER_T         param3;
   DAQ_REGISTER_T         param4;
} DAQ_OPERATION_IO_T;
#ifndef __DOXYGEN__
STATIC_ASSERT( sizeof(DAQ_OPERATION_IO_T) == (sizeof(DAQ_CHANNEL_LOCATION_T)
                                            + 4 * sizeof( DAQ_REGISTER_T ) ));
#endif

/*!
 * @brief Complete operation type which is necessary to perform a
 *        DAQ operation of the LM32 application from the Linux host.
 */
typedef struct PACKED_SIZE
{
   DAQ_OPERATION_CODE_T code;    /*!<@brief OP code */
   DAQ_RETURN_CODE_T    retCode; /*!<@brief Status code */
   DAQ_OPERATION_IO_T   ioData;  /*!<@brief Device and channel selection
                                            and parameter list */
} DAQ_OPERATION_T;
#ifndef __DOXYGEN__
STATIC_ASSERT( sizeof(DAQ_OPERATION_T) == (sizeof(DAQ_OPERATION_CODE_T)
                                         + sizeof(DAQ_RETURN_CODE_T)
                                         + sizeof(DAQ_OPERATION_IO_T) ));
#endif

/*!
 * @brief Final data type in shared memory for DAQ.
 */
typedef struct PACKED_SIZE
{
   /*!
    * @brief Magic number
    */
   uint32_t                 magicNumber;

   /*!
    * @brief Access parameters for the SCU RAM,
    *        for now the DDR3 RAM.
    */
   RAM_RING_SHARED_OBJECT_T ramIndexes;

   /*!
    * @brief Operation parameter to invoke a
    *        LM32 function form the Linux host.
    */
   DAQ_OPERATION_T          operation;
} DAQ_SHARED_IO_T;
#ifndef __DOXYGEN__
STATIC_ASSERT( sizeof( DAQ_SHARED_IO_T ) == (sizeof(uint32_t)
                                           + sizeof(RAM_RING_SHARED_OBJECT_T)
                                           + sizeof(DAQ_OPERATION_T) ));
STATIC_ASSERT( sizeof( DAQ_SHARED_IO_T ) <= SHARED_SIZE );
#endif

/*!
 * @brief Initializer of DAQ shared memory.
 * @see DAQ_SHARED_IO_T
 */
#define DAQ_SHARAD_MEM_INITIALIZER                    \
{                                                     \
   .magicNumber = DAQ_MAGIC_NUMBER,                   \
   .ramIndexes  = RAM_RING_SHARED_OBJECT_INITIALIZER, \
   .operation =                                       \
   {                                                  \
      .code    = DAQ_OP_IDLE,                         \
      .retCode = DAQ_RET_OK                           \
   }                                                  \
}

/*!@} */
#endif /* ifndef _DAQ_COMMAND_INTERFACE_H */
/*================================== EOF ====================================*/