/*!
 * @file scu_msi.h
 * @brief Message-Signaled Interrupts (MSI)
 *
 * @copyright GSI Helmholtz Centre for Heavy Ion Research GmbH
 * @author    Ulrich Becker <u.becker@gsi.de>
 * @date      04.03.2020
 * @see https://www-acc.gsi.de/wiki/Timing/TimingSystemHowSoftCPUHandleECAMSIs
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
#ifndef _SCU_MSI_H
#define _SCU_MSI_H

#include <mini_sdb.h>
#include <lm32Interrupts.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRQ_REG_RST  0x00000000
#define IRQ_REG_STAT 0x00000004
#define IRQ_REG_POP  0x00000008

#define IRQ_OFFS_QUE 0x00000020

#define IRQ_OFFS_MSG 0x00000000
#define IRQ_OFFS_ADR 0x00000004
#define IRQ_OFFS_SEL 0x00000008

/*! ---------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Control registers of Message-Signaled Interrupt (MSI)
 *        wishbone object
 */
typedef struct HW_IMAGE
{
   uint32_t reset;
   uint32_t status;
   uint32_t pop;
} MSI_CONTROL_T;

#ifndef __DOXYGEN__
STATIC_ASSERT( offsetof( MSI_CONTROL_T, reset )  == IRQ_REG_RST );
STATIC_ASSERT( offsetof( MSI_CONTROL_T, status ) == IRQ_REG_STAT );
STATIC_ASSERT( offsetof( MSI_CONTROL_T, pop )    == IRQ_REG_POP );
STATIC_ASSERT( sizeof( MSI_CONTROL_T ) == 3 * sizeof( uint32_t ) );
#endif

/*! ---------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Message-Signaled Interrupt (MSI) message object type which
 *        corresponds to the related interrupt.
 */
typedef struct HW_IMAGE
{
   uint32_t  msg;
   uint32_t  adr;
   uint32_t  sel;
} MSI_ITEM_T;

#ifndef __DOXYGEN__
STATIC_ASSERT( offsetof( MSI_ITEM_T, msg ) == IRQ_OFFS_MSG );
STATIC_ASSERT( offsetof( MSI_ITEM_T, adr ) == IRQ_OFFS_ADR );
STATIC_ASSERT( offsetof( MSI_ITEM_T, sel ) == IRQ_OFFS_SEL );
STATIC_ASSERT( sizeof( MSI_ITEM_T ) == 3 * sizeof( uint32_t ) );
#endif

/*! --------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Message-Signaled Interrupt (MSI) list item object containing
 *        the message object.
 */
typedef struct HW_IMAGE
{
   MSI_ITEM_T item;
   uint32_t   _RFU_;
} MSI_LIST_T;

#ifndef __DOXYGEN__
STATIC_ASSERT( offsetof( MSI_LIST_T, item ) == 0 );
STATIC_ASSERT( sizeof( MSI_LIST_T ) == 4 * sizeof(uint32_t) );
#endif

/*! ---------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Complete hardware image of Message-Signaled Interrupts (MSI)
 *
 * The base address becomes obtained via:
 * @code
 * find_device_adr(GSI, CPU_MSI_CTRL_IF)
 * @endcode
 * @see find_device_adr
 */
typedef struct HW_IMAGE
{
   /*!
    * @brief Control registers
    */
   volatile MSI_CONTROL_T    control;

   /*!
    * @brief May be reserved for future use or still unknown. (Placeholder)
    */
   uint8_t                   _RFU_[IRQ_OFFS_QUE - sizeof( MSI_CONTROL_T )];

   /*!
    * @brief List of message objects related to the corresponding
    *        interrupt numbers.
    * @see MAX_LM32_INTERRUPTS
    */
   const volatile MSI_LIST_T queue[MAX_LM32_INTERRUPTS];
} IRQ_MSI_T;

#ifndef __DOXYGEN__
STATIC_ASSERT( offsetof( IRQ_MSI_T, control ) == 0 );
STATIC_ASSERT( offsetof( IRQ_MSI_T, queue ) == IRQ_OFFS_QUE );
STATIC_ASSERT( sizeof( IRQ_MSI_T ) == IRQ_OFFS_QUE + MAX_LM32_INTERRUPTS * sizeof( MSI_LIST_T ) );
#endif

#if 1
/*! --------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Macro accomplishes a wishbone access to a member of a MSI object.
 *
 * It has a logical corresponding to:
 * @code
 * ((IRQ_MSI_T*)pCpuIrqSlave)->queue[intNum].item.M
 * @endcode
 *
 * @param M Register name
 * @param intNum Interrupt number of the corresponding interrupt.
 */
#define IRQ_MSI_ITEM_ACCESS( M, intNum ) \
   __WB_ACCESS( IRQ_MSI_T, uint32_t, pCpuIrqSlave, queue[intNum].item.M )

/*! --------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Macro accomplishes a wishbone access to a MSI control register.
 *
 * It has a logical corresponding to:
 * @code
 * ((IRQ_MSI_T*)pCpuIrqSlave)->control.M
 * @endcode
 * @param M Register name
 */
#define IRQ_MSI_CONTROL_ACCESS( M ) \
   __WB_ACCESS( IRQ_MSI_T, uint32_t, pCpuIrqSlave, control.M )

#else
#warning Direct wishbone access seems not always work!
#define IRQ_MSI_ITEM_ACCESS( M, i ) \
   ((IRQ_MSI_T*)pCpuIrqSlave)->queue[i].item.M

#define IRQ_MSI_CONTROL_ACCESS( M ) \
   ((IRQ_MSI_T*)pCpuIrqSlave)->control.M
#endif

//int msiGetBoxCpuSlot( int32_t cpuIdx, uint32_t myOffs );

//int msiGetBoxSlot( uint32_t myOffs );

/*! ---------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Removes the Message-Signaled Interrupt object (MSI) form the queue.
 * @param intNum Interrupt number of the corresponding interrupt.
 */
STATIC inline void irqMsiPop( const unsigned int intNum )
{
   IRQ_MSI_CONTROL_ACCESS( pop ) = _irqGetPendingMask( intNum );
}

/*! ---------------------------------------------------------------------------
 * @ingroup INTERRUPT
 * @brief Copies the message object related to the given interrupt number and
 *        removes it from the queue.
 * @param pItem Pointer to target object where the data shall copied.
 * @param intNum Interrupt number of the corresponding interrupt.
 */
STATIC inline void irqMsiCopyObjectAndRemove( MSI_ITEM_T* const pItem,
                                              const unsigned int intNum )
{
   pItem->msg = IRQ_MSI_ITEM_ACCESS( msg, intNum );
   pItem->adr = IRQ_MSI_ITEM_ACCESS( adr, intNum );
   pItem->sel = IRQ_MSI_ITEM_ACCESS( sel, intNum );

   irqMsiPop( intNum );
}


#ifdef __cplusplus
}
#endif
#endif /* ifndef _SCU_MSI_H */
 /*================================= EOF ====================================*/
