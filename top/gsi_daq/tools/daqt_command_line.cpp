/*!
 *  @file daqt_command_line.cpp
 *  @brief Command line parser of DAQ-Test
 *
 *  @date 16.04.2019
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
#include <find_process.h>
#include <unistd.h>
#include <netdb.h>
#include "daqt_command_line.hpp"

using namespace daqt;
using namespace std;

#define CONTINUE_1MS   "1MS"
#define CONTINUE_100US "100US"
#define CONTINUE_10US  "10US"

/*! ---------------------------------------------------------------------------
*/
#define __GET_ATTRIBUTE_PTR()                                                 \
  Attributes* poAttr =                                                        \
     static_cast<CommandLine*>(poParser)->getAttributesToSet();               \
     if( poAttr == nullptr )                                                  \
     {                                                                        \
        specifiedBeforeErrorMessage();                                        \
        return -1;                                                            \
     }

/*! ---------------------------------------------------------------------------
*/
#define __SET_BOOL_ATTRIBUTE( member )                                        \
   __GET_ATTRIBUTE_PTR()                                                      \
   poAttr->member.set( true )

/*! ---------------------------------------------------------------------------
*/
#define __SET_NUM_ATTRIBUTE( member, value )                                  \
   __GET_ATTRIBUTE_PTR()                                                      \
   poAttr->member.set( value )

/*! ---------------------------------------------------------------------------
*/
void CommandLine::specifiedBeforeErrorMessage( void )
{
   ERROR_MESSAGE( "<proto/host/port> must be specified before!" );
}

/*! ---------------------------------------------------------------------------
*/
DaqAdministration* CommandLine::getDaqAdmin( PARSER* poParser )
{
   DaqAdministration* pAdmin = static_cast<CommandLine*>(poParser)->m_poAllDaq;
   if( pAdmin == nullptr )
      specifiedBeforeErrorMessage();
   return pAdmin;
}

/*! ---------------------------------------------------------------------------
*/
vector<OPTION> CommandLine::c_optList =
{
   {
      OPT_LAMBDA( poParser,
      {
         cout << "Usage: " << poParser->getProgramName()
              << " <proto/host/port> [global-options] "
                 "[<slot-number> [device-options] "
                 "<channel-number> [channel-options]] \n\n"
                 "Global-options can be overwritten by device-options "
                 "and device options can be overwritten by channel-options.\n"
                 "NOTE: The lowest slot-number begins at 1; "
                 "the lowest channel-number begins at 1.\n\n"
                 "Hot keys:\n"
              << HOT_KEY_SHOW_STATE << ": Shows the currently configuration\n"
              << HOT_KEY_POST_MORTEM << ": Triggering a post mortem event\n"
              << HOT_KEY_HIGH_RES << ": Triggering a high resolution event\n"
              << HOT_KEY_RESET    << ": Reset\n"
              << HOT_KEY_RECEIVE << ": Toggling receiving on / off\n"
                 "\nCommandline options:\n";
         poParser->list( cout );
         cout << endl;
         ::exit( EXIT_SUCCESS );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'h',
      .m_longOpt  = "help",
      .m_helpText = "Print this help and exit"
   },
   {
      OPT_LAMBDA( poParser,
      {
         cout << TO_STRING( VERSION ) << endl;
         ::exit( EXIT_SUCCESS );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'V',
      .m_longOpt  = "version",
      .m_helpText = "Print the software version and exit"
   },
   {
      OPT_LAMBDA( poParser,
      {
         DaqAdministration* poAllDaq = getDaqAdmin( poParser );
         if( poAllDaq == nullptr )
            return -1;
         if( static_cast<CommandLine*>(poParser)->isVerbose() )
         {
            for( unsigned int i = 1; i <= poAllDaq->getMaxFoundDevices(); i++ )
            {
               cout << "slot " << poAllDaq->getSlotNumber( i )
                    << ": channels: " << poAllDaq->readMaxChannels( i ) << endl;
            }
         }
         else
         {
            for( unsigned int i = 1; i <= poAllDaq->getMaxFoundDevices(); i++ )
            {
               cout << poAllDaq->getSlotNumber( i ) << " "
                    <<  poAllDaq->readMaxChannels( i ) << endl;
            }
         }
         ::exit( EXIT_SUCCESS );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 's',
      .m_longOpt  = "scan",
      .m_helpText = "Scanning of the entire SCU-bus for"
                    " DAQ- devices and its channels and"
                    " print the results as table and exit.\n"
                    "The first column shows the slot number and the second"
                    " column the number of channels.\n"
                    "Output example:\n"
                    "   3 4\n"
                    "   9 4\n"
                    "   Means: DAQ in slot 3 found with 4 channels and "
                    "DAQ in slot 9 found with 4 channels."
   },
   {
      OPT_LAMBDA( poParser,
      {
         __GET_ATTRIBUTE_PTR()
         if( poParser->getOptArg().empty() )
         {
            poAttr->m_continueMode.set( ::DAQ_SAMPLE_1MS );
            return 0;
         }
         if( poParser->getOptArg() == CONTINUE_1MS )
         {
            poAttr->m_continueMode.set( ::DAQ_SAMPLE_1MS );
            return 0;
         }
         if( poParser->getOptArg() == CONTINUE_100US )
         {
            poAttr->m_continueMode.set( ::DAQ_SAMPLE_100US );
            return 0;
         }
         if( poParser->getOptArg() == CONTINUE_10US )
         {
            poAttr->m_continueMode.set( ::DAQ_SAMPLE_10US );
            return 0;
         }
         ERROR_MESSAGE( "Wrong sample parameter: \"" <<
                        poParser->getOptArg() << "\"\n"
                        "Known values: "
                        CONTINUE_1MS ", " CONTINUE_100US " or "
                        CONTINUE_10US );
         return -1;
      }),
      .m_hasArg   = OPTION::OPTIONAL_ARG,
      .m_id       = 0,
      .m_shortOpt = 'C',
      .m_longOpt  = "continue",
      .m_helpText = "Starts the continuous mode \n"
                    "PARAM:\n"
                    "   " CONTINUE_1MS   " Sample rate   1 ms\n"
                    "   " CONTINUE_100US " Sample rate 100 us\n"
                    "   " CONTINUE_10US  " Sample rate  10 us\n"
                    "Default value is 1ms\n"
                    "Example:\n"
                    "   C=" CONTINUE_100US "\n"
                    "   Means: Continuous mode with sample rate of 100 us"

   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_highResolution );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'H',
      .m_longOpt  = "high-resolution",
      .m_helpText = "Starts the high resolution mode"
   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_postMortem );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'P',
      .m_longOpt  = "post-mortem",
      .m_helpText = "Starts the post-mortem mode"
   },
   {
      OPT_LAMBDA( poParser,
      {
         unsigned int limit;
         if( readInteger( limit, poParser->getOptArg() ) )
            return -1;
         if( limit > static_cast<DAQ_REGISTER_T>(~0) )
         {
            ERROR_MESSAGE( "Requested block limit: " << limit <<
                           " is out of range!" );
            return -1;
         }
         __SET_NUM_ATTRIBUTE( m_blockLimit, limit );
         return 0;
      }),
      .m_hasArg   = OPTION::REQUIRED_ARG,
      .m_id       = 0,
      .m_shortOpt = 'l',
      .m_longOpt  = "limit",
      .m_helpText = "Limits the maximum of data blocks in the continuous"
                    "mode.\n"
                    "The value zero (default) establishes the endless mode."
   },
   {
      OPT_LAMBDA( poParser,
      {
         unsigned int delay;
         if( readInteger( delay, poParser->getOptArg() ) )
            return -1;
         if( delay > static_cast<DAQ_REGISTER_T>(~0) )
         {
            ERROR_MESSAGE( "Requested trigger delay: " << delay <<
                           " is out of range!" );
            return -1;
         }
         __SET_NUM_ATTRIBUTE( m_triggerDelay, delay );
         return 0;
      }),
      .m_hasArg   = OPTION::REQUIRED_ARG,
      .m_id       = 0,
      .m_shortOpt = 'd',
      .m_longOpt  = "delay",
      .m_helpText = "PARAM sets the trigger delay in samples"
                    " for the continuous mode"
   },
   {
      OPT_LAMBDA( poParser,
      {
         unsigned int condition;
         if( readInteger( condition, poParser->getOptArg() ) )
            return -1;
         if( condition > static_cast<uint32_t>(~0) )
         {
            ERROR_MESSAGE( "Requested trigger condition: " << condition <<
                           " is out of range!" );
            return -1;
         }
         __SET_NUM_ATTRIBUTE( m_triggerCondition, condition );
         return 0;
      }),
      .m_hasArg   = OPTION::REQUIRED_ARG,
      .m_id       = 0,
      .m_shortOpt = 'c',
      .m_longOpt  = "condition",
      .m_helpText = "PARAM sets the timing value of the trigger condition"
   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_triggerEnable );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 't',
      .m_longOpt  = "trigger",
      .m_helpText = "Enables the trigger mode for continuous- and"
                     " high-resolution mode"
   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_continueTriggerSouce );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'E',
      .m_longOpt  = "continuous-extern",
      .m_helpText = "Sets the trigger source for the continuous mode "
                    "from the event-trigger (default)\n"
                    "into the external trigger input"
   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_highResTriggerSource );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'e',
      .m_longOpt  = "highres-extern",
      .m_helpText = "Sets the trigger source for the high-resolution mode "
                    "from the event-trigger (default)\n"
                    "into the external trigger input"
   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_restart );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'r',
      .m_longOpt  = "restart",
      .m_helpText = "Restarts the high-resolution or post-mortem mode after"
                    " an event"
   },
   {
      OPT_LAMBDA( poParser,
      {
         static_cast<CommandLine*>(poParser)->m_verbose = true;
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'v',
      .m_longOpt  = "verbose",
      .m_helpText = "Be verbose"
   },
   {
      OPT_LAMBDA( poParser,
      {
         __SET_BOOL_ATTRIBUTE( m_zoomGnuPlot );
         return 0;
      }),
      .m_hasArg   = OPTION::NO_ARG,
      .m_id       = 0,
      .m_shortOpt = 'z',
      .m_longOpt  = "zoom",
      .m_helpText = "Zooming of the Y-axis in GNUPLOT."
   },
   {
      OPT_LAMBDA( poParser,
      {
         if( getDaqAdmin( poParser ) == nullptr )
            return -1;

         static_cast<CommandLine*>(poParser)->m_gnuplotBin =
                                                       poParser->getOptArg();
         return 0;
      }),
      .m_hasArg   = OPTION::REQUIRED_ARG,
      .m_id       = 0,
      .m_shortOpt = 'G',
      .m_longOpt  = "gnuplot",
      .m_helpText = "Replacing of the default Gnuplot binary by the in PARAM"
                    " given binary. Default is: " GPSTR_DEFAULT_GNUPLOT_EXE
   },
   {
      OPT_LAMBDA( poParser,
      {
         if( getDaqAdmin( poParser ) == nullptr )
            return -1;

         static_cast<CommandLine*>(poParser)->m_gnuplotTerminal =
                                                       poParser->getOptArg();
         return 0;
      }),
      .m_hasArg   = OPTION::REQUIRED_ARG,
      .m_id       = 0,
      .m_shortOpt = 'T',
      .m_longOpt  = "terminal",
      .m_helpText = "PARAM replaces the terminal which is used by Gnuplot."
                    " Default is: " GNUPLOT_DEFAULT_TERMINAL
   },
   {
      OPT_LAMBDA( poParser,
      {
         if( getDaqAdmin( poParser ) == nullptr )
            return -1;

         static_cast<CommandLine*>(poParser)->m_gnuplotOutput =
                                                       poParser->getOptArg();
         return 0;
      }),
      .m_hasArg   = OPTION::REQUIRED_ARG,
      .m_id       = 0,
      .m_shortOpt = 'o',
      .m_longOpt  = "output",
      .m_helpText = "Setting the prefix and suffix file name for Gnuplot."
                    " PARAM is the path and name of the output file.\n"
                    "NOTE: The final file name becomes generated as follows:\n"
                    "      <SUFFIX>_<SCU-name>_<slot number>_<channel number>_"
                    "<wr-time stamp>.<PREFIX>\n"
                    "Example: PARAM = myFile.png:\n"
                    "         result: myFile_scuxl0035_acc_gsi_de_3_1_"
                    "12439792657334272.png"
   }
};

/*! ---------------------------------------------------------------------------
*/
bool CommandLine::readInteger( unsigned int& rValue, const string& roStr )
{
   try
   {
      rValue = stoi( roStr );
   }
   catch( std::exception& e )
   {
      ERROR_MESSAGE( "Integer number is expected and not that: \""
                     << roStr << "\" !" );
      return true;
   }
   return false;
}

/*! ---------------------------------------------------------------------------
*/
CommandLine::CommandLine( int argc, char** ppArgv )
   :PARSER( argc, ppArgv )
   ,FSM_INIT_FSM( READ_EB_NAME )
   ,m_poAllDaq( nullptr )
   ,m_poCurrentDevice( nullptr )
   ,m_poCurrentChannel( nullptr )
   ,m_verbose( false )
   ,m_gnuplotBin( GPSTR_DEFAULT_GNUPLOT_EXE )
   ,m_gnuplotTerminal( GNUPLOT_DEFAULT_TERMINAL )
{
   add( c_optList );
}

/*! ---------------------------------------------------------------------------
*/
CommandLine::~CommandLine( void )
{
   if( m_poAllDaq != nullptr )
       delete m_poAllDaq;
}

/*! ---------------------------------------------------------------------------
*/
//#define CONFIG_DBG_ATTRIBUTES
DaqContainer* CommandLine::operator()( void )
{
   if( getArgCount() < 2 )
   {
      ERROR_MESSAGE( "Missing argument!" );
      return nullptr;
   }
   if( PARSER::operator()() < 0 )
      return nullptr;
   if( m_poAllDaq == nullptr )
      return nullptr;
   m_poAllDaq->prioritizeAttributes();
#ifdef CONFIG_DBG_ATTRIBUTES
   if( m_poAllDaq->m_oAttributes.m_blockLimit.m_valid )
      cerr << "Attribute: " << m_poAllDaq->m_oAttributes.m_blockLimit.m_value << endl;
   for( auto& iDev: *m_poAllDaq )
   {
      Device* pDev = static_cast<Device*>(iDev);
      cerr << "   Slot: " << pDev->getSlot();
      if( pDev->m_oAttributes.m_blockLimit.m_valid )
         cerr << ", Attribute: " << pDev->m_oAttributes.m_blockLimit.m_value;
      cerr << endl;
      for( auto& iCha: *iDev )
      {
         Channel* pCha = static_cast<Channel*>(iCha);
         cerr << "     Channel: " << pCha->getNumber();
         if( pCha->m_oAttributes.m_blockLimit.m_valid )
            cerr << ", Attribute: " << pCha->m_oAttributes.m_blockLimit.m_value;
         cerr << endl;
      }
   }
#endif
   if( m_poAllDaq->checkWhetherChannelsBecomesOperating() )
      return nullptr;
   if( m_poAllDaq->checkCommandLineParameter() )
      return nullptr;
   if( m_poAllDaq->checkForAttributeConflicts() )
      return nullptr;
   m_poAllDaq->sendAttributes();
   return m_poAllDaq;
}

/*! ---------------------------------------------------------------------------
*/
Attributes* CommandLine::getAttributesToSet( void )
{
   if( m_poAllDaq == nullptr )
      return nullptr;
   if( m_poCurrentChannel != nullptr )
      return &m_poCurrentChannel->m_oAttributes;
   if( m_poCurrentDevice != nullptr )
      return &m_poCurrentDevice->m_oAttributes;
   return &m_poAllDaq->m_oAttributes;
}

/*! ----------------------------------------------------------------------------
 * @brief Callback function of findProcesses in CommandLine::onArgument
 */
extern "C" {
static int onFoundProcess( OFP_ARG_T* pArg )
{  /*
    * Checking whether the found process is himself.
    */
   if( pArg->pid == ::getpid() )
      return 0; // Process has found himself. Program continue.

   string* pEbTarget = static_cast<string*>(pArg->pUser);
   string ebSelfAddr = pEbTarget->substr( pEbTarget->find( '/' ) + 1 );

   const char* currentArg = reinterpret_cast<char*>(pArg->commandLine.buffer);

   /*
    * Skipping over the program name from the concurrent process command line.
    */
   currentArg += ::strlen( currentArg ) + 1;

   for( ::size_t i = 1; i < pArg->commandLine.argc; i++ )
   {
      if( *currentArg != '-' )
         break;
      /*
       * Skipping over possibly leading options from the concurrent
       * process command line.
       */
      currentArg += ::strlen( currentArg ) + 1;
   }

   string ebConcurrentAddr = currentArg;
   ebConcurrentAddr = ebConcurrentAddr.substr( ebConcurrentAddr.find( '/' ) + 1 );

   struct hostent* pHostConcurrent = ::gethostbyname( ebConcurrentAddr.c_str() );
   struct hostent* pHostSelf       = ::gethostbyname( ebSelfAddr.c_str() );
   if( pHostConcurrent == nullptr || pHostSelf == nullptr )
   {
      if( ebConcurrentAddr != ebSelfAddr )
         return 0; // Program continue.
   }
   if( pHostConcurrent != nullptr && pHostSelf != nullptr )
   {
      if( ::strcmp( pHostConcurrent->h_name, pHostSelf->h_name ) != 0 )
         return 0; // Program continue.
   }

   if( (pHostConcurrent != nullptr) != (pHostSelf != nullptr) )
      return 0; // Program continue.

   ERROR_MESSAGE( "A concurrent process accessing \"" << *pEbTarget <<
                  "\" is already running with the PID: " << pArg->pid );

   return -1; // Program termination.
}
} // extern "C"

/*! ---------------------------------------------------------------------------
*/
int CommandLine::onArgument( void )
{
   string arg = getArgVect()[getArgIndex()];
   unsigned int number;
   switch( m_state )
   {
      case READ_SLOT:
      case READ_CHANNEL:
      {
         SCU_ASSERT( m_poAllDaq != nullptr );
         if( readInteger( number, arg ) )
            return -1;
         break;
      }
   }
   switch( m_state )
   {
      case READ_EB_NAME:
      {
         SCU_ASSERT( m_poAllDaq == nullptr );
#if 1
         if( ::findProcesses( getProgramName().c_str(),
                              ::onFoundProcess, &arg,
                              static_cast<FPROC_MODE_T>
                                 (FPROC_BASENAME | FPROC_RLINK) )
             < 0 )
            return -1;
#endif
         m_poAllDaq = new DaqContainer( arg, this );
         FSM_TRANSITION( READ_SLOT );
         break;
      }
      case READ_SLOT:
      {
         m_poCurrentChannel = nullptr;
         if( !gsi::isInRange( number, DaqInterface::c_startSlot,
                                      DaqInterface::c_maxSlots ) )
         {
            ERROR_MESSAGE( "Given slot " << number <<
                           " is out of the range of: " <<
                           DaqInterface::c_startSlot << " and " <<
                           DaqInterface::c_maxSlots << " !" );
            return -1;
         }
         if( !m_poAllDaq->isDevicePresent( number ) )
         {
            ERROR_MESSAGE( "In slot " << number << " isn't a DAQ!" );
            return -1;
         }
         m_poCurrentDevice = m_poAllDaq->getDeviceBySlot( number );
         if( m_poCurrentDevice == nullptr )
         {
            m_poCurrentDevice = new Device( number );
            m_poAllDaq->registerDevice( m_poCurrentDevice );
         }
         FSM_TRANSITION( READ_CHANNEL );
         break;
      }
      case READ_CHANNEL:
      {
         SCU_ASSERT( m_poCurrentDevice != nullptr );
         SCU_ASSERT( m_poCurrentChannel == nullptr );
         if( !gsi::isInRange( number, static_cast<unsigned int>(1),
                                      m_poCurrentDevice->getMaxChannels() ) )
         {
            ERROR_MESSAGE( "Requested channel " << number <<
                           " of DAQ in slot " <<
                           m_poCurrentDevice->getSlot() <<
                           " isn't present!" );
            return -1;
         }
         m_poCurrentChannel = m_poCurrentDevice->getChannel( number );
         if( m_poCurrentChannel == nullptr )
         {
            m_poCurrentChannel = new Channel( number, m_gnuplotBin );
            m_poCurrentDevice->registerChannel( m_poCurrentChannel );
         }
         FSM_TRANSITION( READ_SLOT );
         break;
      }
   }

   return 1;
}

//================================== EOF ======================================
