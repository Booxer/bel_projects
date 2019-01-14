#ifndef _CARPEDM_H_
#define _CARPEDM_H_


#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <map>
#include "common.h"

Class EbWrapper {

protected:

  std::string ebdevname;
  
  void readCycle (const vAdr& va, const vBl& vcs) {
     if (va.size() != vcs.size()) throw std::runtime_error(" EB Read cycle Adr / Flow control vector lengths (" 
      + std::to_string(va.size()) + "/" + std::to_string(vcs.size()) + ") do not match\n");
  }

  void writeCycle(const vAdr& va, const vBuf& vb, const vBl& vcs) {   
    if ( (va.size() != vcs.size()) || ( va.size() * _32b_SIZE_ != vb.size() ) )
    throw std::runtime_error(" EB/sim write cycle Adr / Data / Flow control vector lengths (" + std::to_string(va.size()) + "/" 
      + std::to_string(vb.size() /  _32b_SIZE_) + "/" + std::to_string(vcs.size()) +") do not match\n");
  }

public:
  EbWrapper(const std::string& ebdevname) : ebdevname(ebdevname) {}

  virtual ~EbWrapper() = default;
  virtual bool connect() = 0;
  virtual bool disconnect()  = 0; //Close connection
  virtual int writeCycle(const ebWrs& ew) const = 0;
  virtual int writeCycle(const vAdr& va, const vBuf& vb, const vBl& vcs)  const = 0;
  virtual int writeCycle(const vAdr& va, const vBuf& vb)  const = 0;
  virtual vBuf readCycle(const ebRds& er)  const = 0;
  virtual vBuf readCycle(const vAdr& va, const vBl& vcs) const = 0;
  virtual vBuf readCycle(const vAdr& va ) const = 0;
  virtual uint32_t read32b(uint32_t adr) const = 0;
  virtual uint64_t read64b(uint32_t startAdr) const = 0;
  virtual int write32b(uint32_t adr, uint32_t data) const = 0;
  virtual int write64b(uint32_t startAdr, uint64_t data) const = 0;
  virtual bool isSimulation()  const = 0;
  virtual uint64_t getDmWrTime() const = 0;
  virtual showCpuList() const = 0;
  virtual void showCpuList() const = 0;


};