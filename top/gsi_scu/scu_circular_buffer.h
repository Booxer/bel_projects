/*!
 *  @file scu_circular_buffer.h
 *  @brief SCU- circular buffer resp. ring buffer administration in
 *         shared memory of LM32.
 *
 *  @date 21.10.2019
 *  @copyright (C) 2019 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Stefan Rauch perhaps...
 *  @revision Ulrich Becker <u.becker@gsi.de>
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
#ifndef _SCU_CIRCULAR_BUFFER_H
#define _SCU_CIRCULAR_BUFFER_H

#include <scu_function_generator.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
namespace Scu
{
#endif

typedef uint32_t RING_POS_T;

#ifdef __cplusplus
namespace FG
{
#endif

#define RING_SIZE   64
#define DAQ_RING_SIZE  2048
//#define DAQ_RING_SIZE 512
//#define DAQ_RING_SIZE  1024

/** @brief check if a channel buffer is empty
 *  @param cr channel register
 *  @param channel number of the channel
 */
static
inline bool cbisEmpty(volatile FG_CHANNEL_REG_T* cr, const unsigned int channel)
{
   return cr[channel].wr_ptr == cr[channel].rd_ptr;
}

/** @brief get the fill level  of a channel buffer
 *  @param cr channel register
 *  @param channel number of the channel
 */
static
inline RING_POS_T cbgetCount(volatile FG_CHANNEL_REG_T* cr, const unsigned int channel )
{
   if( cr[channel].wr_ptr > cr[channel].rd_ptr )
      return cr[channel].wr_ptr - cr[channel].rd_ptr;

   if( cr[channel].rd_ptr > cr[channel].wr_ptr )
      return BUFFER_SIZE - cr[channel].rd_ptr + cr[channel].wr_ptr;

   return 0;
}

/** @brief check if a channel buffer is full
 *  @param cr channel register
 *  @param channel number of the channel
 */
static inline bool cbisFull(volatile FG_CHANNEL_REG_T* cr, const unsigned int channel)
{
   return (cr[channel].wr_ptr + 1) % (BUFFER_SIZE) == cr[channel].rd_ptr;
}

/** @brief read a parameter set from a channel buffer
 *  @param pCb pointer to the first channel buffer
 *  @param pCr pointer to the first channel register
 *  @param channel number of the channel
 *  @param pPset the data from the buffer is written to this address
 */
static inline
bool cbRead( volatile FG_CHANNEL_BUFFER_T* pCb, volatile FG_CHANNEL_REG_T* pCr,
            const unsigned int channel, FG_PARAM_SET_T* pPset )
{
   const uint32_t rptr = pCr[channel].rd_ptr;

   /* check empty */
   if( pCr[channel].wr_ptr == rptr )
      return false;
  /* read element */
#ifdef __cplusplus
   //TODO Workaround, I don't know why yet!
   *pPset = *((FG_PARAM_SET_T*) &(pCb[channel].pset[rptr]));
#else
   *pPset = pCb[channel].pset[rptr];
#endif
   /* move read pointer forward */
   pCr[channel].rd_ptr = (rptr + 1) % (BUFFER_SIZE);
   return true;
}

typedef struct PACKED_SIZE
{
   uint32_t  msg;
   uint32_t  adr;
   uint32_t  sel;
} MSI_T;

typedef struct PACKED_SIZE
{
   RING_POS_T ring_head;
   RING_POS_T ring_tail;
   MSI_T      ring_data[RING_SIZE];
} FG_MESSAGE_BUFFER_T;

//#pragma pack(pop)
#ifdef __lm32__
void cbWrite(volatile FG_CHANNEL_BUFFER_T* cb, volatile FG_CHANNEL_REG_T*, const int, FG_PARAM_SET_T*);

void cbDump(volatile FG_CHANNEL_BUFFER_T* cb, volatile FG_CHANNEL_REG_T*, const int num );

int add_msg(volatile FG_MESSAGE_BUFFER_T* mb, int queue, MSI_T m);

MSI_T remove_msg(volatile FG_MESSAGE_BUFFER_T* mb, int queue);

#endif /* ifdef __lm32__ */

/** @brief test if a queue has any messages
 *  @param mb pointer to the first message buffer
 *  @param queue number of the queue
 */
static inline bool has_msg(volatile FG_MESSAGE_BUFFER_T* mb, int queue)
{
   return (mb[queue].ring_head != mb[queue].ring_tail);
}

#ifdef __cplusplus
} /* namespace FG */
namespace MiLdaq
{
#endif

#ifndef CONFIG_MIL_DAQ_USE_RAM

typedef struct PACKED_SIZE
{
   uint32_t   setvalue;
   uint32_t   actvalue;
   uint32_t   tmstmp_l;
   uint32_t   tmstmp_h;
   FG_MACRO_T fgMacro;
} MIL_DAQ_OBJ_T;

typedef struct PACKED_SIZE
{
   RING_POS_T    ring_head;
   RING_POS_T    ring_tail;
   MIL_DAQ_OBJ_T ring_data[DAQ_RING_SIZE];
} MIL_DAQ_BUFFER_T;

#ifdef __lm32__
void add_daq_msg(volatile MIL_DAQ_BUFFER_T* db, MIL_DAQ_OBJ_T d );
#endif

#endif /* ifndef CONFIG_MIL_DAQ_USE_RAM */

#ifdef __cplusplus
} /* namespace MiLdaq */
} /* namespace Scu */
} /* extern "C" */
#endif
#endif /* ifndef _SCU_CIRCULAR_BUFFER_H */
/*================================== EOF ====================================*/