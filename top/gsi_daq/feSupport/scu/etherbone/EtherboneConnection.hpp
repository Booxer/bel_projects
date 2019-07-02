#pragma once

#include <etherbone.h>
#include <utility>      // std::pair, std::make_pair
#include <string>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "feSupport/scu/etherbone/Constants.hpp"

/*! ---------------------------------------------------------------------------
 * @brief Upholstering the object for etherbone/wishbone receiving
 *
 * This macro will used for the function EtherboneConnection::doRead
 * when the last argument "packedTarget" is equal "true".
 *
 * This macro is necessary to prevent a segmentation fault.
 * Because at the moment the etherbone layer supposes in any cases that the
 * target pointer points to a value of type etherbone::data_t. But if the
 * target data type is a smaller type as etherbone::data_t it could crash.
 * In this case the excess bytes will written in a padding variable.
 *
 * @see FeSupport::Scu::Etherbone::EtherboneConnection::doRead
 *
 * @param type Payload data type.
 * @param object Instance name of payload type
 * @return Padded data type
 */
#define EB_PADDING_T( type, object )                                          \
   class                                                                      \
   {                                                                          \
   public:                                                                    \
      type object;                                                            \
   private:                                                                   \
      etherbone::data_t __padding;                                            \
   }


namespace FeSupport {
  namespace Scu {
    /*!
     * Namespace used to organize all etherbone related
     * classes.
     */
    namespace Etherbone {
      namespace IPC = boost::interprocess;
      /*!
       * \brief Connection abstraction for an etherbone bus.
       */
      class EtherboneConnection {
        public:
           typedef IPC::named_mutex MUTEX_T;

          /*!
           * \brief Basic constuctor
           */
          EtherboneConnection(std::string netaddress, unsigned int timeout = 5000);

          /*!
           * \brief Destructor
           */
          virtual ~EtherboneConnection();

          /*!
           * \brief check if named mutex is locked
           * @param true: unlock mutex, false: don't
           */
          bool checkMutex(bool unlock=false);

          /*!
           * \brief Connects to the etherbone bus
           */
          void connect();

          /*!
           * \brief Disconnects from the bus
           */
          void disconnect();
          /*!
           * \brief Searches for a particular device address
           */
          uint64_t findDeviceBaseAddress(Etherbone::VendorId vendorId,
                                         Etherbone::DeviceId deviceId,
                                         uint32_t ind=0);
          /*!
           * \brief Reads a value from the bus in etherbone format. 
           *
           * It wraps the asynchronous
           * etherbone call into a synchronous read method.
           *
           * @param eb_address Address to read from
           * @param data Array to store read data
           * @param format 32 or 16 bit
           * @param size Length of data array
           * @param packedTarget If false (default) the target-pointer (data)
           *                     will added with sizeof( etherbone::data_t )
           *                     for each data word,
           *                     else the target pointer will added with
           *                     the size defined in parameter format.
           * @note CAUTION!\n
           *       packedTarget=true could be dangerous! Because at the moment
           *       the etherbone library supposes in any cases that
           *       the target pointer can take the place of sizeof(etherbone::data_t). \n
           *       If the target-size defined in parameter "format" smaller than
           *       sizeof(etherbone::data_t) a segmentation fault could happen respectively
           *       variables could be overwritten.\n
           *       To prevent this the use of padding space of \n
           *       at least sizeof(etherbone::data_t) - (format & EB_DATAX)
           *       becomes necessary.
           *
           * @see EB_PADDING_T
           */
          void doRead(etherbone::address_t eb_address, etherbone::data_t* data,
                      etherbone::format_t format,
                      const uint16_t size = 1,
                      bool packedTarget = false);

          void doVectorRead(const etherbone::address_t &eb_address, 
                            std::vector  <std::pair <etherbone::data_t, etherbone::data_t> >& v,
                            etherbone::format_t format);

          /*!
           * \brief Writes a single etherbone value to the bus. 
           *
           * It wraps the asynchronous etherbone
           * call into a synchronous write method.
           *
           * @param eb_address Address to write to
           * @param data Array of data to write
           * @param format 32 or 16 bit
           * @param size Length of data array
           * @param packedSource If false (default) a source data array of type
           *                     etherbone::data_t is expected, otherwise a source
           *                     data array of a type-size defined in argument "format"
           *                     is expected.
           */
          void doWrite(const etherbone::address_t eb_address,
                       const etherbone::data_t* data,
                       etherbone::format_t format,
                       const uint16_t size = 1,
                       bool packedSource = false
                      );

          void doVectorWrite(const etherbone::address_t &eb_address, 
                             const std::vector <std::pair <etherbone::data_t, etherbone::data_t> >& v,
                             etherbone::format_t format);

          /*!
           * \brief Activate debugging outputs on console. 
           *
           * @param activate/deactivate debug output to console
           */
          void setDebug(const bool deb) { debug_ = deb; }

          uint32_t getSlaveMacroVersion(VendorId vendorId, DeviceId deviceId);

          /*!
           * @brief Returns true if connection has been successful established.
           * @author UB
           */
          bool isConnected() const
          {
             return connectionOpened;
          }

          /*!
           * @brief Returns the net address given as first argument of
           *        the constructor.
           * @author UB
           */
          const std::string& getNetAddress()
          {
             return netaddress_;
          }

#ifdef CONFIG_VIA_EB_CYCLE
          /*!
           * @brief Returns a reverence to the whishbone/etherbone device.
           * @author UB
           */
          etherbone::Device& getEbDevice()
          {
             return eb_device_;
          }

          /*!
           * @brief Returns a reverence to the system-mutex;
           * @author UB
           */
          MUTEX_T& getMutex()
          {
             return _sysMu;
          }
#endif
        private:
          /*!
           * @brief System mutex
           */
          MUTEX_T _sysMu;
          // EB-Device to talk to
          //
          std::string netaddress_;

          // How long we want to wait for eb to answer [ms] TODO NOT USED!
          unsigned int timeout_;

          // TODO Add RAII, initialize eb things in ctor
          // The socket, used for this connection
          etherbone::Socket eb_socket_;

          // device to talk too
          etherbone::Device eb_device_;

          // This flag shows weather the connection was opened
          bool connectionOpened;

          // Contains the error Message in case the operation fails
          std::string errorMessage_;

          // debug flag, when set detailed infos for each etherbone access
          // will be printed to console output
          bool debug_;
      };

    } // namespace Etherbone 
  } // namespace Scu
} // namepace FeSupport


