/*!
 * @file scu_fg_feedback.hpp
 * @brief Administration of data aquesition units for function generator
 *        feedback.
 *
 * @date 25.05.2020
 * @copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 * @author Ulrich Becker <u.becker@gsi.de>
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>
 ******************************************************************************
 */
#ifndef _SCU_FG_FEEDBACK_HPP
#define _SCU_FG_FEEDBACK_HPP

#include <list>
#include <scu_control_config.h>
#include <daq_calculations.hpp>
#include <daq_administration.hpp>
#ifdef CONFIG_MIL_FG
 #include <mdaq_administration.hpp>
#endif
#include <scu_fg_list.hpp>
#include <daq_base_interface.hpp>

namespace Scu
{
using namespace gsi;

class FgFeedbackDevice;
class FgFeedbackAdministration;

///////////////////////////////////////////////////////////////////////////////
/*!
 * @brief Object type of feedback-channel for all DAQ-types.
 *
 * Polymorphic object type, after registration in FgFeedbackDevice it
 * converts to a MIL- or to a ADDAC/ACU- feedback channel, depending whether
 * the FgFeedbackDevice is a MIL or ADDAC/ACU device.
 */
class FgFeedbackChannel
{
   friend class FgFeedbackDevice;

   /*!
    * @brief Common object type for ADDAC/ACU- and MIL- feedback channel
    */
   class Common
   {
   protected:
      FgFeedbackChannel* m_pParent;

   public:
      Common( FgFeedbackChannel* pParent );
      virtual ~Common( void );
   }; // class Common

   /*!
    * @brief Object type handling ADDAC/ACU-DAQ set- and actual- channel.
    */
   class AddacFb: public Common
   {
      friend class FgFeedbackDevice;

      /*!
       * @brief Object type containing the data buffer for received actual or
       *        set values of a ADDAC-DAQ data block.
       */
      class Receive: public daq::DaqChannel
      {
         AddacFb*        m_pParent;
         uint64_t        m_timestamp;
         uint            m_sampleTime;
         std::size_t     m_blockLen;
         uint8_t         m_sequence;
         daq::DAQ_DATA_T m_aBuffer[daq::DaqAdministration::c_contineousDataLen];

      public:
         Receive( AddacFb* pParent, const uint n );
         virtual ~Receive( void );

         uint64_t getTimestamp( void ) const
         {
            return m_timestamp;
         }

         uint const getSampleTime( void ) const
         {
            return m_sampleTime;
         }

         uint8_t getSequence( void ) const
         {
            return m_sequence;
         }

         std::size_t getBlockLen( void ) const
         {
            return m_blockLen;
         }

         daq::DAQ_DATA_T operator[]( const uint i ) const
         {
            assert( i < ARRAY_SIZE(m_aBuffer) );
            return m_aBuffer[i];
         }

      protected:
         bool onDataBlock( daq::DAQ_DATA_T* pData, std::size_t wordLen ) override;
         void onInit( void ) override;
         void onReset( void ) override;
      }; // class Receive

      /*!
       * @brief Data buffer of the last received set values.
       */
      Receive m_oReceiveSetValue;

      /*!
       * @brief Data buffer of the last received actual values.
       */
      Receive m_oReceiveActValue;

   public:
      AddacFb( FgFeedbackChannel* pParent );
      virtual ~AddacFb( void );

   private:
      void finalizeBlock( void );
   }; // class AddacFb

#ifdef CONFIG_MIL_FG
   /*!
    * @brief Object type handling MIL- channel.
    */
   class MilFb: public Common
   {
      friend class FgFeedbackDevice;

      /*!
       * @brief Object type to forwarding incoming MIL-data
       *        to a higher software layer.
       */
      class Receive: public MiLdaq::DaqCompare
      {
         MilFb*  m_pParent;
      public:
         Receive( MilFb* pParent );
         virtual ~Receive( void );
         void onData( uint64_t wrTimeStampTAI,
                      MiLdaq::MIL_DAQ_T actlValue,
                      MiLdaq::MIL_DAQ_T setValue ) override;
         void onInit( void ) override;
         void onReset( void ) override;
      }; // class Receive

      Receive m_oReceive;

   public:
      MilFb( FgFeedbackChannel* pParent );
      virtual ~MilFb( void );
   }; // class MilFb
#endif // ifdef CONFIG_MIL_FG

   const uint         m_fgNumber;
   FgFeedbackDevice*  m_pParent;
   Common*            m_pCommon;

public:
   /*!
    * @brief Constructor of a single function generator feedback channel.
    * @param fgNumber Number of function generator.
    */
   FgFeedbackChannel( const uint fgNumber )
      :m_fgNumber( fgNumber )
      ,m_pParent( nullptr )
      ,m_pCommon( nullptr )
   {
   }

   virtual ~FgFeedbackChannel( void );

   FgFeedbackDevice* getParent( void );

   /*!
    * @brief Returns the function generator number.
    */
   uint getFgNumber( void ) const
   {
      return m_fgNumber;
   }

   uint getSocket( void );

#ifdef CONFIG_MIL_FG
   bool isMil( void );
#endif

protected:
   /*!
    * @brief Callback function becomes invoked for each incoming data item which
    *        belongs to this object.
    * @param wrTimeStampTAI White rabbit time stamp TAI.
    * @param actlValue Actual value from the DAQ.
    * @param setValue Set value from function generator
    */
   virtual void onData( uint64_t wrTimeStampTAI,
                        MiLdaq::MIL_DAQ_T actlValue,
                        MiLdaq::MIL_DAQ_T setValue ) = 0;

   /*!
    * @brief Optional callback function becomes invoked once this object
    *        is registered in its container of type DaqDevice and this
    *        container is again registered in the administrator
    *        object of type DaqAdministration.
    */
   virtual void onInit( void ) {}

   /*!
    * @brief Optional callback function becomes invoked by a reset event.
    */
   virtual void onReset( void ) {}

}; // class FgFeedbackChannel

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*!
 * @brief Object type for MIL-or ADDAC/ACU devices.
 *
 * Polymorphic object type, depending on the socket number
 * - the constructors argument - it converts to a MIL- or to a ADDAC/ACU-
 * device.
 */
class FgFeedbackDevice
{
   friend class FgFeedbackAdministration;

   using CHANNEL_LIST_T = std::list<FgFeedbackChannel*>;

   DaqBaseDevice*            m_poDevice;
   FgFeedbackAdministration* m_pParent;
   CHANNEL_LIST_T            m_lChannelList;

public:
   FgFeedbackDevice( const uint socket );
   ~FgFeedbackDevice( void );

   FgFeedbackAdministration* getParent( void );

   void registerChannel( FgFeedbackChannel* pFeedbackChannel );

   void unregisterChannel( FgFeedbackChannel* pFeedbackChannel ) {/*TODO*/}

   /*!
    * @brief Returns the socket number.
    *
    * It is the constructors argument.
    */
   uint getSocket( void ) const
   {
      return m_poDevice->getSocket();
   }

   /*!
    * @brief Returns the SCU- bus slot number occupying this device.
    */
   uint getSlot( void ) const
   {
      return m_poDevice->getSlot();
   }

#ifdef CONFIG_MIL_FG
   /*!
    * @brief Returns the pointer to the MIL device if this object has been
    *        mutated to a MIL- object, else NULL.
    */
   MiLdaq::DaqDevice* getMil( void ) const
   {
      return dynamic_cast<MiLdaq::DaqDevice*>(m_poDevice);
   }

   /*!
    * @brief Returns "true" if this object has been mutated to a MIL- object,
    *        else "false".
    */
   bool isMil( void ) const
   {
      return (getMil() != nullptr);
   }
#endif

   /*!
    * @brief Returns the pointer to the ADDAC/ACU device if this object
    *        has been mutated to a ADDAC/ACU- object, else NULL.
    */
   daq::DaqDevice* getAddac( void ) const
   {
      return dynamic_cast<daq::DaqDevice*>(m_poDevice);
   }

   /*!
    * @brief Returns "true" if this object has been mutated to a
    *        ADDAC/ACU- object, else "false".
    */
   bool isAddac( void ) const
   {
      return (getAddac() != nullptr);
   }

   /*!
    * @brief Returns the pointer to the channel object tu which belongs
    *        the given number
    * @param nunber Channel number
    * @retval !=nullptr Pointer of channel object
    * @retval ==nullptr Chnnel not present respectively not registered.
    */
   FgFeedbackChannel* getChannel( const uint number );

   const CHANNEL_LIST_T::iterator begin( void )
   {
      return m_lChannelList.begin();
   }

   const CHANNEL_LIST_T::iterator end( void )
   {
      return m_lChannelList.end();
   }
}; // class FgFeedbackDevice

#ifdef CONFIG_MIL_FG
inline   bool FgFeedbackChannel::isMil( void )
{
  return getParent()->isMil();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*!
 * @brief Object type for the feedback administration of all types of
 *        SCU function generators
 */
class FgFeedbackAdministration
{
   using DAQ_POLL_T     = std::vector<DaqBaseInterface*>;
   using GEN_DEV_LIST_T = std::list<FgFeedbackDevice*>;

   /*!
    * @brief List of function generators found by the LM32 application.
    */
   FgList m_oFoundFgs;

   /*!
    * @brief Object type for ADDAC-DAQ administration
    */
   class AddacAdministration: public daq::DaqAdministration
   {
      FgFeedbackAdministration* m_pParent;
   public:
      AddacAdministration( FgFeedbackAdministration* pParent, DaqEb::EtherboneConnection* poEtherbone )
        :daq::DaqAdministration( poEtherbone, false, false )
        ,m_pParent( pParent )
      {
      }

      AddacAdministration( FgFeedbackAdministration* pParent, daq::EbRamAccess* poEbAccess )
        :daq::DaqAdministration( poEbAccess, false, false )
        ,m_pParent( pParent )
      {
      }
   }; // class AddacAdministration
   /*!
    * @brief Object for ADDAC DAQ administration.
    */
   AddacAdministration     m_oAddacDaqAdmin;

#ifdef CONFIG_MIL_FG
   /*!
    * @brief Object type for MIL-DAQ administration
    */
   class MilDaqAdministration: public MiLdaq::DaqAdministration
   {
      FgFeedbackAdministration* m_pParent;
   public:
      MilDaqAdministration( FgFeedbackAdministration* pParent, daq::EbRamAccess* poEbAccess )
        :MiLdaq::DaqAdministration( poEbAccess )
        ,m_pParent( pParent )
      {
      }
      virtual ~MilDaqAdministration( void ) {}

      void onUnregistered( RingItem* pUnknownItem )  override
      {
         //TODO
      }

      RAM_RING_INDEX_T getCurrentRamSize( bool update = true ) override
      {
         return 0;
      }

      void clearBuffer( bool update = true ) override
      {
         //TODO
      }
   }; // class MilDaqAdministration

   /*!
    * @brief Object for MIL DAQ administration.
    */
   MilDaqAdministration  m_oMilDaqAdmin;
#endif

   /*!
    * @brief Object triggering software interrupts to LM32-firmware
    */
   Lm32Swi                    m_lm32Swi;

   DAQ_POLL_T                 m_vPollList;
   GEN_DEV_LIST_T             m_lDevList;

protected:
   #define DEVICE_LIST_BASE std::list
   using DEVICE_LIST_T = DEVICE_LIST_BASE<FgFeedbackDevice*>;
   DEVICE_LIST_T m_devicePtrList;

public:
   FgFeedbackAdministration( DaqEb::EtherboneConnection* poEtherbone, const bool doRescan = false );
   FgFeedbackAdministration( daq::EbRamAccess* poEbAccess, const bool doRescan = false );
   virtual ~FgFeedbackAdministration( void );

   /*!
    * @brief Returns the SCU LAN domain name or the name of the wishbone
    *        device.
    */
   const std::string getScuDomainName( void )
   {
      return m_oAddacDaqAdmin.getScuDomainName();
   }

   /*!
    * @brief returns a pointer to an object of type daq::EbRamAccess
    */
   daq::EbRamAccess* getEbAccess( void )
   {
      return m_oAddacDaqAdmin.getEbAccess();
   }

   /*!
    * @brief Returns the major version number of the
    *        LM32 firmware after a scan has been made.
    */
   uint getLm32SoftwareVersion( void ) const
   {
      return m_oFoundFgs.getLm32SoftwareVersion();
   }

   /*!
    * @brief Triggering a software interrupt in LM32 firmware
    */
   void sendSwi( FG::FG_OP_CODE_T opCode, uint param = 0 )
   {
      m_lm32Swi.send( opCode, param );
   }

   /*!
    * @brief Scanning and synchronizing of the function-generator list found
    *        by the LM32 application.
    * @note This function performances a re-scan by the LM32!
    */
   void scan( const bool doRescan = false );

   /*!
    * @brief Synchronizing of the function-generator list found by the
    *        LM32 application.
    */
   void sync( void )
   {
      m_oFoundFgs.sync( m_oAddacDaqAdmin.getEbAccess() );
   }

   /*!
    * @brief Returns a reference to the function generator list.
    */
   FgList& getFgList( void )
   {
      return m_oFoundFgs;
   }

   /*!
    * @brief Returns the number of found MIL function generators after
    *        a scan has been made.
    */
   uint getNumOfFoundMilFg( void )
   {
      return m_oFoundFgs.getNumOfFoundMilFg();
   }

   /*!
    * @brief Returns the number of ADDAC and/or ACO function generators after
    *        a scan has been made.
    */
   uint getNumOfFoundNonMilFg( void )
   {
      return m_oFoundFgs.getNumOfFoundNonMilFg();
   }

   /*!
    * @brief Returns the total number of found function generators after
    *        a scan has been made.
    */
   uint getNumOfFoundFg( void )
   {
      return m_oFoundFgs.getNumOfFoundFg();
   }

   /*!
    * @brief Returns true if function generator with
    *        the given socket and given device number present.
    * @note A scan of function generators before assumed!
    */
   bool isPresent( const uint socket, const uint device )
   {
      return m_oFoundFgs.isPresent( socket, device );
   }

   /*!
    * @brief Returns true if the given socket number is used by a
    *        function generator.
    * @note A scan of function generators before assumed!
    */
   bool isSocketUsed( const uint socket )
   {
      return m_oFoundFgs.isSocketUsed( socket );
   }

   /*!
    * @brief Returns a pointer to a registered device object.
    * @param socket Device number
    * @retval !=nullptr Pointer to the device object
    * @retval ==nullptr Device not present respectively not registered.
    */
   FgFeedbackDevice* getDevice( const uint socket );

   /*!
    * @brief Registering of a device containing function generators.
    * @note If the given device or one of its containing function generators
    *       are not present on the SCU, an exception will throw.
    */
   void registerDevice( FgFeedbackDevice* poDevice );

   void unregisterDevice( FgFeedbackDevice* poDevice ) {/*TODO*/}

   const GEN_DEV_LIST_T::iterator begin( void )
   {
      return m_lDevList.begin();
   }

   const GEN_DEV_LIST_T::iterator end( void )
   {
      return m_lDevList.end();
   }

   /*!
    * @brief Central polling routine of all feedback channels.
    *
    * This function checks whether data from a DAQ channel in the appropriate
    * shared LM32-memory and - if there - invokes the on "onData" function of
    * the associated channel object.
    *
    * @note This function should run in a polling-loop of a own thread.
    */
   void distributeData( void );

   void reset( void );
}; // class FgFeedbackAdministration

///////////////////////////////////////////////////////////////////////////////
/*! ---------------------------------------------------------------------------
 */
inline uint FgFeedbackChannel::getSocket( void )
{
   return getParent()->getSocket();
}



} // End namespace Scu

#endif // ifndef _SCU_FG_FEEDBACK_HPP
//================================== EOF ======================================