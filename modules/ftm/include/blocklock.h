#ifndef _BLOCK_LOCK_H_
#define _BLOCK_LOCK_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <etherbone.h>
#include "common.h"
#include "hashmap.h"
#include "alloctable.h"
#include "covenanttable.h"


struct LockFlags {
  bool set, clr, stat, rdy;
}

class BlockLock {
private:
  const uint32_t baseAdr, qflagsAdr, wridxAdr, rdidxAdr;

public:
  const std::string name;
  LockFlags rd;
  LockFlags wr;

  BlockLock(const std:string& name, const uint32_t adr) 
    : name(name), adr(baseAdr), qflagsAdr(adr + BLOCK_CMDQ_FLAGS), 
    wridxAdr(adr + BLOCK_CMDQ_WR_IDXS), rdidxAdr(adr + BLOCK_CMDQ_RD_IDXS) {};
  
  BlockLock(const std:string& name, const uint32_t adr, bool setDNR, bool clrDNR, bool setDNW, bool clrDNW)
    : name(name), adr(baseAdr), qflagsAdr(adr + BLOCK_CMDQ_FLAGS), 
    wridxAdr(adr + BLOCK_CMDQ_WR_IDXS), rdidxAdr(adr + BLOCK_CMDQ_RD_IDXS), 
    rd.set(setDNR), rd.clr(clrDNR), wr.set(setDNW), wr.clr(clrDNW) {};
 
  ~BlockLock();


  vEbwrs& lock(vEbwrs& ew) {;

  vEbwrs& unlock(vEbwrs& ew );

  vEbrds& updateStat(vEbrds& er );

  vEbrds& updateAll(vEbrds& er );

  bool isLockSet();

  bool isLockAct();

};  

 

#endif