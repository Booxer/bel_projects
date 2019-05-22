/*!
 *  @file daq_ramBuffer.c
 *  @brief Abstraction layer for handling RAM buffer for DAQ data blocks.
 *
 *  @see scu_ramBuffer.h
 *
 *  @see scu_ddr3.h
 *  @see scu_ddr3.c
 *  @date 07.02.2019
 *  @copyright (C) 2019 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Ulrich Becker <u.becker@gsi.de>
 *
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
#include <dbg.h>
#include <eb_console_helper.h>
#include <daq_ramBuffer.h>

/*! ---------------------------------------------------------------------------
 * @see scu_ramBuffer.h
 */
RAM_RING_INDEX_T ramRingGetSize( const RAM_RING_INDEXES_T* pThis )
{
   if( pThis->end == pThis->capacity) /* Is ring-buffer full? */
      return pThis->capacity;
   if( pThis->end >= pThis->start )
      return pThis->end - pThis->start;
//   mprintf( "***\n" );
   return (pThis->capacity - pThis->start) + pThis->end;
}

/*! ---------------------------------------------------------------------------
 * @see scu_ramBuffer.h
 */
void ramRingAddToWriteIndex( RAM_RING_INDEXES_T* pThis, RAM_RING_INDEX_T toAdd )
{
   RAM_ASSERT( ramRingGetRemainingCapacity( pThis ) >= toAdd );
   RAM_ASSERT( pThis->end < pThis->capacity );

   pThis->end = (pThis->end + toAdd) % pThis->capacity;

   if( pThis->end == pThis->start )
      pThis->end = pThis->capacity; /* Ring buffer is full. */
}

/*! ---------------------------------------------------------------------------
 * @see scu_ramBuffer.h
 */
void ramRingAddToReadIndex( RAM_RING_INDEXES_T* pThis, RAM_RING_INDEX_T toAdd )
{
   RAM_ASSERT( ramRingGetSize( pThis ) >= toAdd );

   /* Is ring-buffer full? */
   if( (toAdd != 0) && (pThis->end == pThis->capacity) )
      pThis->end = pThis->start;

   pThis->start = (pThis->start + toAdd) % pThis->capacity;
}

//#define CONFIG_DAQ_DEBUG

#ifdef CONFIG_DAQ_DEBUG
/*! ---------------------------------------------------------------------------
 * @brief Prints the values of the members of RAM_RING_INDEXES_T
 */
  #ifndef __lm32__
    #define mprintf printf
  #endif
  static
  void ramRingDbgPrintIndexes( const RAM_RING_INDEXES_T* pThis, const char* txt )
  {
     if( txt != NULL )
       mprintf( "DBG: %s\n", txt );
     mprintf( "  DBG: offset:   %d\n"
              "  DBG: capacity: %d\n"
              "  DBG: start:    %d\n"
              "  DBG: end:      %d\n"
              "  DBG: used:     %d\n"
              "  DBG: free      %d\n\n",
              pThis->offset,
              pThis->capacity,
              pThis->start,
              pThis->end,
              ramRingGetSize( pThis ),
              ramRingGetRemainingCapacity( pThis )
            );
  }
#else
  #define ramRingDbgPrintIndexes( __a, __b ) ((void)0)
#endif

//////////////////////////////////////////////////////////////////////////////

/*! ---------------------------------------------------------------------------
 * @see scu_ramBuffer.h
 */
int ramInit( register RAM_SCU_T* pThis, RAM_RING_SHARED_OBJECT_T* pSharedObj
          #ifdef __linux__
           , EB_HANDLE_T* pEbHandle
          #endif
           )
{
   pThis->pSharedObj = pSharedObj;
   ramRingReset( &pSharedObj->ringIndexes );
#ifdef CONFIG_SCU_USE_DDR3
 #ifdef __lm32__
   return ddr3init( &pThis->ram );
 #else
   return ddr3init( &pThis->ram, pEbHandle );
 #endif
#endif
}

/*! ---------------------------------------------------------------------------
 * @brief Generalized function to read a item from the ring buffer.
 */
static inline
void ramRreadItem( register RAM_SCU_T* pThis, const RAM_RING_INDEX_T index,
                   RAM_DAQ_PAYLOAD_T* pItem )
{
#ifdef CONFIG_SCU_USE_DDR3
   ddr3read64( &pThis->ram, pItem, index );
#else
   #error Nothing implemented in function ramRreadItem()!
#endif
}

#if defined(__lm32__) || defined(__DOXYGEN__)

/*! ---------------------------------------------------------------------------
 * @brief Macro ease the read access to the channel mode register in the
 *        device descriptor storing as data block in the DAQ-RAM.
 * @see _DAQ_CHANNEL_CONTROL
 * @see _DAQ_DISCRIPTOR_STRUCT_T
 * @see RAM_DAQ_PAYLOAD_T
 * @param item RAM payload item including the channel mode register of
 *             the device descriptor.
 */
#define RAM_ACCESS_CHANNEL_MODE( item )                                       \
(                                                                             \
   *((_DAQ_CHANNEL_CONTROL*)&item.ad16                                        \
   [                                                                          \
      (RAM_DAQ_INDEX_OFFSET_OF_CHANNEL_CONTROL %                              \
       offsetof(_DAQ_DISCRIPTOR_STRUCT_T, cControl )) /                       \
       sizeof(DAQ_DATA_T)                                                     \
   ])                                                                         \
)
/* Awful I know... */

/*! ---------------------------------------------------------------------------
 * @brief Definition of return values of function ramRingGetTypeOfOldestBlock
 */
typedef enum
{
   RAM_DAQ_EMPTY,     //!<@brief No block present
   RAM_DAQ_UNDEFINED, //!<@brief No block recognized
   RAM_DAQ_SHORT,     //!<@brief Short block
   RAM_DAQ_LONG       //!<@brief Long block
} RAM_DAQ_BLOCK_T;

/*! ---------------------------------------------------------------------------
 * @see RAM_DAQ_BLOCK_T
 */
static inline
RAM_DAQ_BLOCK_T ramRingGetTypeOfOldestBlock( register RAM_SCU_T* pThis )
{
   unsigned int size = ramRingGetSize( &pThis->pSharedObj->ringIndexes );
   if( size == 0 )
      return RAM_DAQ_EMPTY;

   if( (size % RAM_DAQ_SHORT_BLOCK_LEN) != 0 )
   {
      DBPRINT1( ESC_FG_RED ESC_BOLD
                "DBG: ERROR: RAM content not dividable by "
                "minimum block length!\n"ESC_NORMAL );
      return RAM_DAQ_UNDEFINED;
   }

   RAM_DAQ_PAYLOAD_T  item;
   RAM_RING_INDEXES_T indexes = pThis->pSharedObj->ringIndexes;
   ramRingAddToReadIndex( &indexes, RAM_DAQ_INDEX_OFFSET_OF_CHANNEL_CONTROL );
   ramRreadItem( pThis, ramRingGeReadIndex( &indexes ), &item );

#if  (DEBUGLEVEL>1)
   for( unsigned int i = 0; i < ARRAY_SIZE(item.ad16); i++ )
      DBPRINT2( "DBG: item: %d 0x%04x %d\n", i, item.ad16[i], item.ad16[i] );

   DBPRINT2( "DBG: daq:   0x%x\n", RAM_ACCESS_CHANNEL_MODE( item ).daqMode );
   DBPRINT2( "DBG: pm:    0x%x\n", RAM_ACCESS_CHANNEL_MODE( item ).pmMode );
   DBPRINT2( "DBG: hiRes: 0x%x\n", RAM_ACCESS_CHANNEL_MODE( item ).hiResMode );
#endif

   /*
    * Rough check of the device descriptors integrity.
    */
   if( ((int)RAM_ACCESS_CHANNEL_MODE( item ).daqMode)   +
       ((int)RAM_ACCESS_CHANNEL_MODE( item ).hiResMode) +
       ((int)RAM_ACCESS_CHANNEL_MODE( item ).pmMode)    != 1 )
   {
       DBPRINT1( ESC_FG_RED ESC_BOLD
                "DBG: ERROR: RAM wrong modes!\n"
                 ESC_NORMAL);
       return RAM_DAQ_UNDEFINED;
   }

   if( RAM_ACCESS_CHANNEL_MODE( item ).daqMode )
      return RAM_DAQ_SHORT;

   return RAM_DAQ_LONG;
}

/*! ---------------------------------------------------------------------------
 * @brief Removes the oldest DAQ- block in the ring boffer
 */
static inline
void ramRemoveOldestBlock( register RAM_SCU_T* pThis )
{
   switch( ramRingGetTypeOfOldestBlock( pThis ) )
   {
      case RAM_DAQ_UNDEFINED:
      {
         ramRingReset( &pThis->pSharedObj->ringIndexes );
         break;
      }
      case RAM_DAQ_SHORT:
      {
         ramRingAddToReadIndex( &pThis->pSharedObj->ringIndexes,
                                RAM_DAQ_SHORT_BLOCK_LEN );
         break;
      }
      case RAM_DAQ_LONG:
      {
         ramRingAddToReadIndex( &pThis->pSharedObj->ringIndexes,
                                RAM_DAQ_LONG_BLOCK_LEN );
         break;
      }
      default: break;
   }
}

/*! ---------------------------------------------------------------------------
 * @brief Checks whether a additional DAQ-block can stored in the ring buffer.
 */
inline static
bool ramDoesBlockFit( register RAM_SCU_T* pThis, const bool isShort )
{
   return (ramRingGetRemainingCapacity( &pThis->pSharedObj->ringIndexes ) >=
           (isShort? RAM_DAQ_SHORT_BLOCK_LEN : RAM_DAQ_LONG_BLOCK_LEN));
}

/*! ---------------------------------------------------------------------------
 * @brief Removes the oldest blocks in ring buffer until it is enough space
 *        for a new Block.
 * @note If the blocks are not correctly recognized so the entire ring buffer
 *       becomes deleted.
 */
inline static
void ramMakeSpaceIfNecessary( register RAM_SCU_T* pThis, const bool isShort )
{
   while( !ramDoesBlockFit( pThis, isShort ) )
   {
      DBPRINT1( "DBG: "ESC_FG_YELLOW"Removing block!\n"ESC_NORMAL );
      ramRemoveOldestBlock( pThis );
   }
}

/*! ---------------------------------------------------------------------------
 */
static inline ALWAYS_INLINE
void ramWriteItem( register RAM_SCU_T* pThis, const RAM_RING_INDEX_T index,
                   RAM_DAQ_PAYLOAD_T* pItem )
{
#ifdef CONFIG_SCU_USE_DDR3
   ddr3write64( &pThis->ram, index, pItem );
#else
   #error Nothing implemented in function ramWriteItem()!
#endif
}


/*! ---------------------------------------------------------------------------
 * @brief Helper function for ramWriteDaqData
 */
static inline ALWAYS_INLINE
void ramFillItem( RAM_DAQ_PAYLOAD_T* pItem, const unsigned int i,
                  const DAQ_DATA_T data )
{
#ifdef CONFIG_SCU_USE_DDR3
   RAM_ASSERT( i < ARRAY_SIZE( pItem->ad16 ) );
   ramSetPayload16( pItem, data, i );
#else
   #error Nothing implemented in function ramFillItem()!
#endif
}

#ifdef CONFIG_DEBUG_RAM_WRITE_DATA
  #define DBG_RAM_INFO DBPRINT1
#else
  #define DBG_RAM_INFO( ... )
#endif

/*! ---------------------------------------------------------------------------
 * @brief Publishing written data in shared memory.
 */
static inline
void publishWrittenData( register RAM_SCU_T* pThis,
                         RAM_RING_INDEXES_T* poIndexes )
{
   pThis->pSharedObj->ringIndexes.end = poIndexes->end;
   pThis->pSharedObj->serverHasWritten = 1;
   DBG_RAM_INFO( "DBG: RAM-items: %d\n",
                 ramRingGetSize( &pThis->pSharedObj->ringIndexes ) );
}

/*! ---------------------------------------------------------------------------
 */
static inline void ramPollAccessLock( RAM_SCU_T* pThis )
{
#ifdef CONFIG_DEBUG_RAM_WRITE_DATA
   if( pThis->pSharedObj->ramAccessLock )
   {
      unsigned int pollCount = 0;
      DBG_RAM_INFO( ESC_FG_MAGNETA ESC_BOLD
                    "DBG: Enter RAM-access polling\n"
                    ESC_NORMAL );
      while( pThis->pSharedObj->ramAccessLock )
      {
         pollCount++;
      }
      DBG_RAM_INFO( ESC_FG_MAGNETA ESC_BOLD
                    "DBG: Leaving RAM-access polling. %d loops\n"
                    ESC_NORMAL, pollCount );
   }
#else
   while( pThis->pSharedObj->ramAccessLock ) {}
#endif
}

//#define CONFIG_DAQ_DECREMENT

/*! ---------------------------------------------------------------------------
 * @brief Copies the data of the given DAQ-channel in to the RAM and
 *        exchanges the order of DAQ data with device descriptor.
 */
static inline
void ramWriteDaqData( register RAM_SCU_T* pThis, DAQ_CANNEL_T* pDaqChannel,
                      bool isShort )
{
   DAQ_REGISTER_T (*getRemaining)( register DAQ_CANNEL_T* );
   volatile DAQ_DATA_T (*pop)( register DAQ_CANNEL_T* );

   uint8_t*     pSequence;
   DAQ_REGISTER_T remainingDataWords;
   unsigned int dataWordCounter;
   unsigned int payloadIndex;
   DAQ_REGISTER_T expectedWords;
   unsigned int di;

   DAQ_DESCRIPTOR_T    oDescriptor;
   RAM_RING_INDEXES_T  oDescriptorIndexes;
   RAM_RING_INDEXES_T  oDataIndexes;
   RAM_RING_INDEXES_T* poIndexes;

   RAM_DAQ_PAYLOAD_T ramItem;
   DAQ_DATA_T        firstData[RAM_DAQ_DESCRIPTOR_COMPLETION];

   oDescriptorIndexes = pThis->pSharedObj->ringIndexes;
   oDataIndexes       = oDescriptorIndexes;
   poIndexes          = &oDataIndexes;

   /*
    * Skipping over the intended place of the device descriptor.
    */
   ramRingAddToWriteIndex( poIndexes, RAM_DAQ_DATA_START_OFFSET );

   ramRingDbgPrintIndexes( &pThis->pSharedObj->ringIndexes, "Origin indexes:");
   ramRingDbgPrintIndexes( poIndexes, "Data indexes:" );

   DBG_RAM_INFO( "DBG: %s() : Slot: %d, Channel: %d\n", __func__,
             daqChannelGetSlot( pDaqChannel ),
             daqChannelGetNumber( pDaqChannel ) + 1 );

   if( isShort )
   {
      getRemaining  = daqChannelGetDaqFifoWords;
      pop           = daqChannelPopDaqFifo;
      expectedWords = DAQ_FIFO_DAQ_WORD_SIZE_CRC;
      pSequence     = &pDaqChannel->sequenceContinuous;
   }
   else
   {
      getRemaining  = daqChannelGetPmFifoWords;
      pop           = daqChannelPopPmFifo;
      expectedWords = DAQ_FIFO_PM_HIRES_WORD_SIZE_CRC;
      pSequence     = &pDaqChannel->sequencePmHires;
   }

#ifndef CONFIG_DAQ_DECREMENT
   /*
    * The data wort which includes the CRC isn't a part of the fifo content,
    * therefore we have to add it here.
    */
   remainingDataWords = getRemaining( pDaqChannel ) + 1;
   if( remainingDataWords != expectedWords )
   {
      DBPRINT1( ESC_BOLD ESC_FG_RED
                "DBG ERROR: remainingDataWords != expectedWords\n"
                "           remainingDataWords: %d\n"
                "           expectedWords:      %d\n"
                ESC_NORMAL,
                remainingDataWords,
                expectedWords );
      daqChannelSetStatus( pDaqChannel, DAQ_RECEIVE_STATE_DATA_LOST );
      return;
   }
#endif
   di = 0;
   dataWordCounter = 0;
   do
   {
   #ifdef CONFIG_DAQ_DECREMENT
      remainingDataWords = getRemaining( pDaqChannel );
   #else
      remainingDataWords--;
   #endif
      DAQ_DATA_T data = pop( pDaqChannel );

      if( dataWordCounter < ARRAY_SIZE( firstData ) )
      { /*
         * The first two received data words will stored in a temporary buffer.
         * They will copied in the place immediately after the device
         * descriptor. This manner making the intended RAM- place
         * of the device descriptor dividable by RAM_DAQ_PAYLOAD_T.
         */
         firstData[dataWordCounter] = data;
      #ifdef CONFIG_DAQ_DECREMENT
         DBG_RAM_INFO( "DBG: Words in Fifo: %d\n", remainingDataWords );
      #endif
      }
      else
      {
         if( dataWordCounter == ARRAY_SIZE( firstData ) )
         {
            payloadIndex = 0;
         }

         if( poIndexes == &oDescriptorIndexes )
         { /*
            * Descriptor becomes received.
            */
            RAM_ASSERT( di < ARRAY_SIZE(oDescriptor.index) );
            if( di == offsetof(_DAQ_DISCRIPTOR_STRUCT_T, crcReg ) /
                               sizeof(DAQ_DATA_T) )
            { /*
               * Setting of the sequence number in the device descriptor.
               * Sequence number has been already incremented before in
               * function ramPushDaqDataBlock(), therefore the 1 must be
               * deducted here again.
               */
               ((_DAQ_BF_CRC_REG*)&data)->sequence = *pSequence - 1;
            }
            oDescriptor.index[di++] = data;
         }

         ramFillItem( &ramItem, payloadIndex, data );

         /*
          * Was the last data word of payload received?
          */
         if( remainingDataWords == DAQ_DESCRIPTOR_WORD_SIZE )
         { /*
            * Yes, possibly completion of the last RAM item if necessary.
            * This will be the case, by receiving a short block its
            * total length isn't dividable by RAM_DAQ_PAYLOAD_T.
            */
            while( payloadIndex < (RAM_DAQ_DATA_WORDS_PER_RAM_INDEX-1) )
            {
               payloadIndex++;
               DBG_RAM_INFO( "DBG: Complete with dummy data %d\n",
                             payloadIndex );
               ramFillItem( &ramItem, payloadIndex, 0xCAFE );
            }
         }
         /*
          * Has the block been received completely?
          */
         else if( remainingDataWords == 0 )
         { /*
            * Yes, but because the length of the device descriptor isn't
            * dividable by RAM_DAQ_PAYLOAD_T so the rest becomes filled with
            * the first received data words.
            */
            for( unsigned int i = 0; i < ARRAY_SIZE( firstData ); i++ )
            {
               DBG_RAM_INFO( "DBG: Finalize with first data %d\n", i );
               payloadIndex++;
               RAM_ASSERT( payloadIndex < RAM_DAQ_DATA_WORDS_PER_RAM_INDEX );
               ramFillItem( &ramItem, payloadIndex, firstData[i] );
            }
         }

         /*
          * Next RAM item completed?
          */
         if( payloadIndex == (RAM_DAQ_DATA_WORDS_PER_RAM_INDEX-1) )
         {
            payloadIndex = 0;
            /*
             * Store item in RAM.
             */
            ramWriteItem( pThis, ramRingGetWriteIndex(poIndexes), &ramItem );
            ramRingAddToWriteIndex( poIndexes, 1 );

            /*
             * Is the next data word the first word of the device descriptor?
             */
            if( remainingDataWords == DAQ_DESCRIPTOR_WORD_SIZE )
            { /*
               * Yes, skipping back at the begin.
               */
               poIndexes = &oDescriptorIndexes;
            }
         }
         else
            payloadIndex++;
      }

      dataWordCounter++;
      if( dataWordCounter > expectedWords )
         break;
   }
   while( remainingDataWords > 0 );

   /*
    * Is the block integrity given?
    */
   if( (dataWordCounter == expectedWords)
#ifndef CONFIG_DAQ_DECREMENT
       && (getRemaining( pDaqChannel ) == 0)
#endif
       && daqDescriptorVerifyMode( &oDescriptor )
       && (isShort == daqDescriptorIsShortBlock( &oDescriptor )) )
   { /*
      * Block integrity seems to be okay.
      * Making the new received data block in ring buffer valid.
      */
      publishWrittenData( pThis, &oDataIndexes );
      DBPRINT1( "DBG: Sequence: %d\n", *pSequence - 1 );
      if( pDaqChannel->properties.restart )
      {
         if( daqDescriptorWasHiRes( &oDescriptor ) )
         {
            DBG_RAM_INFO( "DBG: Restarting High Resolution\n" );
            daqChannelEnableHighResolution( pDaqChannel );
         }
         else if( daqDescriptorWasPM( &oDescriptor ) )
         {
            DBG_RAM_INFO( "DBG: Restarting Post Mortem\n" );
            daqChannelEnablePostMortem( pDaqChannel );
         }
      }
   }
#ifdef DEBUGLEVEL
   else
   { /*
      * Block integrity is corrupt!
      */
      daqChannelSetStatus( pDaqChannel, DAQ_RECEIVE_STATE_CORRUPT_BLOCK );
   #ifndef CONFIG_DAQ_DECREMENT
      if( getRemaining( pDaqChannel ) != 0 )
      {
         DBPRINT1( ESC_BOLD ESC_FG_RED
                   "DBG ERROR: PmHires fifo: %d\n"
                   ESC_NORMAL, getRemaining( pDaqChannel ) );
      }
      else
   #endif
         DBPRINT1( ESC_BOLD ESC_FG_RED
                   "DBG ERROR: dataWordCounter > expectedWords\n"
                   "           dataWordCounter: %d\n"
                   "           expectedWords:   %d\n"
                   ESC_NORMAL,
                   dataWordCounter,
                   expectedWords );

   }
#endif /* ifdef DEBUGLEVEL */
   ramRingDbgPrintIndexes( &pThis->pSharedObj->ringIndexes,
                           ESC_FG_WHITE ESC_BOLD "Final indexes" ESC_NORMAL );
   daqDescriptorPrintInfo( &oDescriptor );
}

/*! ---------------------------------------------------------------------------
 * @see scu_ramBuffer.h
 */
int ramPushDaqDataBlock( register RAM_SCU_T* pThis, DAQ_CANNEL_T* pDaqChannel,
                         bool isShort )
{
   RAM_ASSERT( pThis != NULL );
   RAM_ASSERT( pDaqChannel != NULL );

   /*
    * Sequence number becomes incremented here in any cases,
    * so the possible lost of blocks can be detected by the Linux-host.
    *
    * Following debug error messages can be happen, when the external
    * interrupts follows to fast.
    */
   if( isShort )
   {
      pDaqChannel->sequenceContinuous++;
      if( daqChannelGetDaqFifoWords( pDaqChannel ) < DAQ_DESCRIPTOR_WORD_SIZE )
      {
         DBPRINT1( ESC_BOLD ESC_FG_RED
                   "DBG ERROR: Short block < descriptor size!\n"
                   ESC_NORMAL );
         daqChannelSetStatus( pDaqChannel, DAQ_RECEIVE_STATE_CORRUPT_BLOCK );
         return -1;
      }
#ifdef CONFIG_DAQ_SIMULATE_CHANNEL
      daqDescriptorSetPM( &pDaqChannel->simulatedDescriptor, false );
      daqDescriptorSetHiRes( &pDaqChannel->simulatedDescriptor, true );
      daqDescriptorSetDaq( &pDaqChannel->simulatedDescriptor, false );
#endif
   }
   else
   {
      pDaqChannel->sequencePmHires++;
      if( daqChannelGetPmFifoWords( pDaqChannel ) < DAQ_DESCRIPTOR_WORD_SIZE )
      {
         DBPRINT1( ESC_BOLD ESC_FG_RED
                   "DBG ERROR: Long block < descriptor size!\n"
                   ESC_NORMAL );
         daqChannelSetStatus( pDaqChannel, DAQ_RECEIVE_STATE_CORRUPT_BLOCK );
         return -1;
      }
#ifdef CONFIG_DAQ_SIMULATE_CHANNEL
      daqDescriptorSetPM( &pDaqChannel->simulatedDescriptor, false );
      daqDescriptorSetHiRes( &pDaqChannel->simulatedDescriptor, false );
      daqDescriptorSetDaq( &pDaqChannel->simulatedDescriptor, true );
#endif
   }

   ramPollAccessLock( pThis );
   ramMakeSpaceIfNecessary( pThis, isShort );
   ramWriteDaqData( pThis, pDaqChannel, isShort );
   return 0;
}

#endif /* if defined(__lm32__) || defined(__DOXYGEN__) */

#if defined(__linux__) || defined(__DOXYGEN__)
/*! ---------------------------------------------------------------------------
 */
int ramReadDaqDataBlock( register RAM_SCU_T* pThis, RAM_DAQ_PAYLOAD_T* pData,
                         unsigned int len, RAM_DAQ_POLL_FT poll )
{
#ifdef CONFIG_SCU_USE_DDR3
   int ret;
   RAM_RING_INDEXES_T indexes = pThis->pSharedObj->ringIndexes;
   unsigned int lenToEnd = indexes.capacity - indexes.start;

   if( lenToEnd < len )
   {
      ret = ddr3FlushFiFo( &pThis->ram, ramRingGeReadIndex( &indexes ),
                           lenToEnd, pData, poll );
      if( ret != EB_OK )
         return ret;
      ramRingAddToReadIndex( &indexes, lenToEnd );
      len   -= lenToEnd;
      pData += lenToEnd;
   }

   ret = ddr3FlushFiFo( &pThis->ram, ramRingGeReadIndex( &indexes ),
                        len, pData, poll );
   if( ret != EB_OK )
      return ret;
   ramRingAddToReadIndex( &indexes, len );
   pThis->pSharedObj->ringIndexes = indexes;

   return EB_OK;
#else
#error Unknown memory type for function: ramReadDaqDataBlock()
#endif
}

#endif /* defined(__linux__) || defined(__DOXYGEN__) */

/*================================== EOF ====================================*/