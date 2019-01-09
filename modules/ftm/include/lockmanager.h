#ifndef _LOCK_MANAGER_H_
#define _LOCK_MANAGER_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <etherbone.h>
#include "common.h"
#include "hashmap.h"
#include "alloctable.h"
#include "covenanttable.h"
#include "blocklock.h"


    //save lock states -> set requested locks -> send commands -> restore previous lock states
    
    //if both set AND clr bits are true, previous state is restored. Otherwise, set / clr override previous lock state
    //this is used to enable explicit lock or unlock commands

    //Behaviour Table for rd and wr locks: 
    // old set clr    new
    // 0   0   0      0   // -> old state
    // 0   0   1      0   // -> cleared
    // 0   1   0      1   // -> set
    // 0   1   1      0   // -> old state
    // 1   0   0      1   // -> old state
    // 1   0   1      0   // -> cleared
    // 1   1   0      1   // -> set
    // 1   1   1      1   // -> old state

class LockManager {
private:
  etherbone::Device& ebd;
  std::vector<BlockLock> vBL;
  bool sim;
  CovenantTable& ct;
  AllocTable& at;

  uint32_t getNodeAdr(const std:string& name);
  bool hasCovenant(const std:string& name);

  };

public:

  LockManager(etherbone::Device& ebd, bool simulation=false, HashMap& hm, CovenantTable& ct, AllocTable& at) : ebd(ebd), sim(simulation), hm(hm), ct(ct), at(at) {};
  ~LockManager(){};

  BlockLock& add(const std:string& name) {
    if(hasCovenant(name)) throw std::runtime_error("Locking block <" + name + "> would violate a safe2remove-covenant!");
    vBl.push_back(BlockLock(name, getNodeAdr(name)));
    return vBL.back();
  }
  BlockLock& add(const std:string& name, bool setDNR, bool clrDNR, bool setDNW, bool clrDNW) {
    if(hasCovenant(name)) throw std::runtime_error("Locking block <" + name + "> would violate a safe2remove-covenant!");
    vBl.push_back(BlockLock(name, getNodeAdr(name), setDNR, clrDNR, setDNW, clrDNW));
    return vBL.back();
  }
  //not necessary to remove individuals
  void processLockRequests();
  void processUnlockRequests();
  void initStatValues() {

  }
  bool isReady();
 
  

  void clear() {vBL.clear(); };

};  

#endif