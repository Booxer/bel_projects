/*!
 *  @file mdaqt_command_line.hpp
 *  @brief Command line parser of MIL-DAQ-Test
 *
 *  @date 26.08.2019
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
#ifndef _MDAQT_COMMAND_LINE_HPP
#define _MDAQT_COMMAND_LINE_HPP
#ifndef __DOCFSM__
 #include "daqt_messages.hpp"
 #include "parse_opts.hpp"
 #include "mdaqt.hpp"
 #include "gnuplotstream.hpp"
#endif

using namespace CLOP;

namespace Scu
{
namespace MiLdaq
{
namespace MiLdaqt
{

#ifdef FSM_DECLARE_STATE
   #undef  FSM_DECLARE_STATE
#endif
#define FSM_DECLARE_STATE( state, attr... ) state

class MilDaqAdministration; // Loest Henne-Ei Problem...
class Device;
class DaqMilCompare;

///////////////////////////////////////////////////////////////////////////////
class CommandLine: public PARSER
{
   enum STATE_T
   {
      FSM_DECLARE_STATE( READ_EB_NAME ),
      FSM_DECLARE_STATE( READ_SLOT ),
      FSM_DECLARE_STATE( READ_CHANNEL )
   };

   static std::vector<OPTION> c_optList;
   STATE_T                    m_state;
   bool                       m_verbose;
   bool                       m_autoBuilding;
   bool                       m_deviationEnable;
   bool                       m_continuePlotting;

   MilDaqAdministration*      m_poAllDaq;
   Device*                    m_poCurrentDevice;
   DaqMilCompare*             m_poCurrentChannel;

   std::string                m_gnuplotBin;
   std::string                m_gnuplotTerminal;
   std::string                m_gnuplotOutput;

   static bool readInteger( uint&, const std::string& );

public:
   CommandLine( int argc, char** ppArgv );
   virtual ~CommandLine( void );

   MilDaqAdministration* operator()( void );

   int onArgument( void ) override;

   bool isVerbose( void ) const
   {
      return m_verbose;
   }

   const std::string& getGnuplotBinary( void )
   {
      return m_gnuplotBin;
   }

   const std::string& getTerminal( void )
   {
      return m_gnuplotTerminal;
   }

   const std::string& getOutputName( void )
   {
      return m_gnuplotOutput;
   }

   bool isOutputFileDefined( void )
   {
      return !m_gnuplotOutput.empty();
   }

   bool isAutoBuilding( void ) const
   {
      return m_autoBuilding;
   }

   bool isDeviationPlottingEnabled( void ) const
   {
      return m_deviationEnable;
   }

   bool isContinuePlottingEnabled( void ) const
   {
      return m_continuePlotting;
   }

protected:
   int onErrorUnrecognizedShortOption( char unrecognized ) override;
   int onErrorUnrecognizedLongOption( const std::string& unrecognized ) override;
};

} // namespace MiLdaqt
} // namespace MiLdaq
} // namespace Scu


#endif
// ================================= EOF ======================================
