#ifndef _EBWRAPPER_H_
#define _EBWRAPPER_H_
#define ETHERBONE_THROWS 1

#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <map>
#include "common.h"
#include <etherbone.h>
#include "alloctable.h"
#include "ftm_common.h"

#define SDB_VENDOR_GSI      0x0000000000000651ULL
#define SDB_DEVICE_LM32_RAM 0x54111351
#define SDB_DEVICE_DIAG     0x18060200




using namespace etherbone;

class EbWrapper {

protected:
  //bool& sim;
  std::ostream& sLog;
  std::ostream& sErr;
  bool& verbose;
  bool& debug;

  Socket ebs;
  Device ebd;

  std::vector<struct sdb_device> cpuDevs;
  std::vector<struct sdb_device> cluTimeDevs;
  std::vector<struct sdb_device> diagDevs;

  std::vector<int> vFw;
  std::map<uint8_t, uint8_t> cpuIdxMap;
  uint8_t cpuQty;

  std::string ebdevname;

  vStrC vFwIdROM;
  
  void checkReadCycle (const vAdr& va, const vBl& vcs) {
     if (va.size() != vcs.size()) throw std::runtime_error(" EB Read cycle Adr / Flow control vector lengths (" 
      + std::to_string(va.size()) + "/" + std::to_string(vcs.size()) + ") do not match\n");
  }

  void checkWriteCycle(const vAdr& va, const vBuf& vb, const vBl& vcs) {   
    if ( (va.size() != vcs.size()) || ( va.size() * _32b_SIZE_ != vb.size() ) )
    throw std::runtime_error(" EB/sim write cycle Adr / Data / Flow control vector lengths (" + std::to_string(va.size()) + "/" 
      + std::to_string(vb.size() /  _32b_SIZE_) + "/" + std::to_string(vcs.size()) +") do not match\n");
  }

public:
  EbWrapper(std::ostream& sLog, std::ostream& sErr, bool& verbose, bool& debug) : sLog(sLog), sErr(sErr), verbose(verbose), debug(debug) {};
  ~EbWrapper();
  bool connect(const std::string& ebdevname, AllocTable& atUp, AllocTable& atDown);
  bool disconnect(); //Close connection
  int writeCycle(const vEbwrs& ew);
  int writeCycle(const vAdr& va, const vBuf& vb, const vBl& vcs) ;
  int writeCycle(const vAdr& va, const vBuf& vb) ;
  vBuf readCycle(const vEbrds& er) ;
  vBuf readCycle(const vAdr& va, const vBl& vcs);
  vBuf readCycle(const vAdr& va );
  uint32_t read32b(uint32_t adr);
  uint64_t read64b(uint32_t startAdr);
  int write32b(uint32_t adr, uint32_t data);
  int write64b(uint32_t startAdr, uint64_t data);
  //bool isSimulation() ;
  uint64_t getDmWrTime();
  void showCpuList();
  const std::string getFwIdROM(uint8_t cpuIdx);
  const std::string createFwVersionString(const int fwVer);
  int parseFwVersionString(const std::string& s);
  //returns firmware version as int <xxyyzz> (x Major Version, y Minor Version, z Revison; negative values for error codes)
  int getFwVersion(const std::string& fwIdROM);
  const std::string readFwIdROMTag(const std::string& tag, size_t maxlen, bool stopAtCr );
  // SDB Functions
  bool isValidDMCpu(uint8_t cpuIdx) {return (cpuIdxMap.count(cpuIdx) > 0);}; //Check if CPU is registered as running a valid firmware
  int getCpuQty()   const {return cpuQty;} //Return number of found CPUs (not necessarily valid ones!)
  bool isCpuIdxValid(uint8_t cpuIdx) { if ( cpuIdxMap.find(cpuIdx) != cpuIdxMap.end() ) return true; else return false;}
  uint32_t getDiagDevAdr() {return diagDevs[0].sdb_component.addr_first;}

};

#endif