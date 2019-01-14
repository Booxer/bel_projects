#include <thread>
#include <chrono>
#include "lockmanager.h"

uint32_t LockManager::getBaseAdr(const std::string& name) {
  auto it           = at.lookupHash(hm.lookup(name, "getBaseAdr: unknown target "));
  auto* x           = (AllocMeta*)&(*it);
  uint32_t adrBase  = at.adrConv(AdrType::MGMT, AdrType::EXT, x->cpu, x->adr);
  return adrBase;
}

bool LockManager::hasCovenant(const std::string& name) {
  //cmI x = ct.lookup(name);
  return false; //(ct.isOk(x) && isCovenantPending(x)); FIXME: there is no way to access this function atm, its part of safe2remove. static ?
}

BlockLock& LockManager::add(const std::string& name) {
  if(hasCovenant(name)) {throw std::runtime_error("Locking block <" + name + "> would violate a safe2remove-covenant!");}
  vBl.push_back(BlockLock(name, getBaseAdr(name)));
  return vBl.back();
}
/*
BlockLock& LockManager::add(const std::string& name, bool setDNR, bool clrDNR, bool setDNW, bool clrDNW) {
  if(hasCovenant(name)) throw std::runtime_error("Locking block <" + name + "> would violate a safe2remove-covenant!");
  vBl.push_back(BlockLock(name, getBaseAdr(name), setDNR, clrDNR, setDNW, clrDNW));
  return vBl.back();
}
*/

void LockManager::processLockRequests() {
  vEbwrs ew;
  for (auto& l : vBl) { l.lock(ew); }
  ebd.writeCycle(ew.va, ew.vb, ew.vcs );  
}

void LockManager::processUnlockRequests() {
  vEbwrs ew;
  for (auto& l : vBl) { l.unlock(ew); }
  ebd.writeCycle(ew.va, ew.vb, ew.vcs );  
}

void LockManager::readInStat() {
  vEbrds er;
  for (auto& l : vBl) { l.updateStat(er);} //prepare addresses to read
  vBuf b = ebd.readCycle(er.va, er.vcs); // read stat

  //decode read data
  for(unsigned idx = 0; idx < vBl.size(); idx++) {
    uint32_t flags   = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&b[idx * _32b_SIZE_]);
    vBl[idx].wr.stat = (bool)(flags >> BLOCK_CMDQ_DNW_POS); //wr lock is set when flag bit is present
    vBl[idx].rd.stat = (bool)(flags >> BLOCK_CMDQ_DNR_POS); //rd lock is set when flag bit is present
  }
}


bool LockManager::isReady() {
    vEbrds erStat, erAct;
    bool rdy = true;
    
    //prepare addresses to read
    for (auto& l : vBl) { l.updateStat(erStat); l.updateAct(erAct); }
      
    // read stat and first pass act
    vEbrds er = erStat + erAct;
    vBuf b0 = ebd.readCycle(er.va, er.vcs);
    vBuf bStat0(b0.begin(), b0.begin() + vBl.size());
    vBuf bAct0(b0.begin() + vBl.size(), b0.end());

    // probably not necessary to sleep because the eb call takes about as long,
    // but lets not rely on exec time! We got to make sure the DM has finished
    // all changes it started before lock bits were visible.
    std::chrono::milliseconds timespan(5);
    std::this_thread::sleep_for(timespan);
   
    // read second pass act
    vBuf bAct1 = ebd.readCycle(erAct.va, erAct.vcs);

    //decode read data
    for(unsigned idx = 0; idx < vBl.size(); idx++) {
      uint32_t flags  = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&bStat0[idx * _32b_SIZE_]);
      
      uint32_t wrIdx0 = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&bAct0[(idx * 2 + 0) * _32b_SIZE_]);
      uint32_t rdIdx0 = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&bAct0[(idx * 2 + 1) * _32b_SIZE_]);

      uint32_t wrIdx1 = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&bAct1[(idx * 2 + 0) * _32b_SIZE_]);
      uint32_t rdIdx1 = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&bAct1[(idx * 2 + 1) * _32b_SIZE_]);

      vBl[idx].wr.stat = (bool)(flags >> BLOCK_CMDQ_DNW_POS); //wr lock is set when flag bit is present
      vBl[idx].wr.act  = wrIdx0 == wrIdx1;                    //wr lock is active when no index diff was detected
      vBl[idx].rd.stat = (bool)(flags >> BLOCK_CMDQ_DNR_POS); //rd lock is set when flag bit is present
      vBl[idx].rd.act  = rdIdx0 == rdIdx1;                    //rd lock is active when no index diff was detected

      rdy &= ~(vBl[idx].isAllSet() & vBl[idx].isAct());  // if any requested lock was not set or not active, we're not ready.
    }
    
    return rdy;

  }


