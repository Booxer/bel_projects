
uint32_t LockManager::getBaseAdr(const std::string& target) {
  auto it           = at.lookupHash(hm.lookup(target, "getBaseAdr: unknown target "););
  auto* x           = (AllocMeta*)&(*it);
  uint32_t adrBase  = at.adrConv(AdrType::MGMT, AdrType::EXT, x->cpu, x->adr);
  return adrBase;
}

 //check for covenants
    if(optimisedS2R) {
      cmI x = ct.lookup(targetName);
      if (ct.isOk(x) && isCovenantPending(x)) {
        throw std::runtime_error("Locking block <" + targetName + "> for modification would violate a safe2remove-covenant!");
      }

    }

bool LockManager::blockIsLockActive(const std::string& targetName, bool checkReadLock, bool checkWriteLock) {
  if (!(checkReadLock & checkWriteLock)) throw std::runtime_error("Lockcheck on <" + targetName + ">: valid inputs are read, write, or both. None is not permitted.");  

  uint32_t hash       = hm.lookup(targetName, "queueLock: unknown target ");
  auto it             = atDown.lookupHash(hash, carpeDMcommand::exIntro);
  auto* x             = (AllocMeta*)&(*it);
  uint32_t adrBase    = atDown.adrConv(AdrType::MGMT, AdrType::EXT, x->cpu, x->adr);
  uint32_t adrWrIdxs  = adrBase + BLOCK_CMDQ_WR_IDXS;
  uint32_t adrRdIdxs  = adrBase + BLOCK_CMDQ_RD_IDXS;
  uint32_t adrQFlags  = adrBase + BLOCK_CMDQ_FLAGS;
  uint32_t qFlags     = ebReadWord(ebd, adrQFlags) & BLOCK_CMDQ_FLGS_SMSK;
  
  //save lock state bits for others to use later
  bool wrLockState = (qFlags >> BLOCK_CMDQ_DNW_POS) & 1;
  bool rdLockState = (qFlags >> BLOCK_CMDQ_DNR_POS) & 1;

  //check if any of the requested lock bits is not present. If so, return false.
  if( !(((rdLockState && checkReadLock) || !checkReadLock) && ((wrLockState && checkWriteLock) || !checkWriteLock)) ) return false;

  //confirm spin lock
  //cumbersome version for now
  uint32_t oldRdIdxs, newRdIdxs = -1; // set out of range initial value on purpose so initial comp. fails
  uint32_t oldWrIdxs, newWrIdxs = -1; // ""
  uint32_t attempts = 0, maxAttempts = 10;

  bool spin = true;
  // spin lock. check indices of attempted locks until no more changes are detected 
  while(spin) {
    if (attempts == maxAttempts) return false;
    //sleep 0.001;
    spin = false;
    // check read indices if read lock is active
    if(qFlags & ((uint32_t)checkReadLock << BLOCK_CMDQ_DNR_POS)) {  
      oldRdIdxs = newRdIdxs; //save RdIdxs
      newRdIdxs = ebReadWord(ebd, adrRdIdxs);
      spin |= (oldRdIdxs != newRdIdxs);
    }
    // check write indices if wr lock is active
    if(qFlags & ((uint32_t)checkWriteLock << BLOCK_CMDQ_DNW_POS)) {  
      oldWrIdxs = newWrIdxs; //save WrIdxs
      newWrIdxs = ebReadWord(ebd, adrWrIdxs);
      spin |= (oldWrIdxs != newWrIdxs);
    }
    attempts++;
  }

  return true;
}

bool LockManager::hasCovenant(const std::string& targetName) {
  cmI x = ct.lookup(targetName);
  return (ct.isOk(x) && isCovenantPending(x));
}


void LockManager::processLockRequests() {
  vEbwrs ew;

  for (auto& lock : vBl) {

  }
}
void LockManager::processUnlockRequests() {}
void LockManager::initLockValues() {}


bool LockManager::isReady() {
    vEbrds erStat, erAct;


    bool rdy = true;
    
    //prepare addresses to read
    for (auto& lock : vBL) { lock.updateStat(erStat); lock.updateAct(erAct); }
      
    // read stat and first pass act  
    vBuf b0 = ebReadCycle(ebd, erStat + erAct);
    vBuf bStat0(b0.begin(), b0.begin() + vBL.size());
    vBuf bAct0(b0.begin() + vBL.size(), b0.end());

    // probably not necessary to sleep because the eb call takes about as long,
    // but lets not rely on exec time! We got to make sure the DM has finished
    // all changes it started before lock bits were visible.
    std::chrono::milliseconds timespan(5);
    std::this_thread::sleep_for(timespan);
   
    // read second pass act
    vBuf bAct1 = ebReadCycle(ebd, erAct);

    //decode read data
    for(unsigned idx = 0; idx < bStat0.size(); idx += _32b_SIZE_) {
      uint32_t flags  = writeBeBytesToLeNumber((uint8_t*)&bStat0[idx]);
      
      uint32_t wrIdx0 = writeBeBytesToLeNumber((uint8_t*)&bAct0[(idx + 0 * _32b_SIZE_) * 2]);
      uint32_t rdIdx0 = writeBeBytesToLeNumber((uint8_t*)&bAct0[(idx + 1 * _32b_SIZE_) * 2]);
      
      uint32_t wrIdx1 = writeBeBytesToLeNumber((uint8_t*)&bAct1[(idx + 0 * _32b_SIZE_) * 2]);
      uint32_t rdIdx1 = writeBeBytesToLeNumber((uint8_t*)&bAct1[(idx + 1 * _32b_SIZE_) * 2]);


      lock.wr.stat = (bool)(flags >> BLOCK_CMDQ_DNW_POS); //wr lock is set when flag bit is present
      lock.wr.act  = wrIdx0 == wrIdx1;                    //wr lock is active when no index diff was detected
      lock.rd.stat = (bool)(flags >> BLOCK_CMDQ_DNR_POS); //rd lock is set when flag bit is present
      lock.rd.act  = rdIdx0 == rdIdx1;                    //rd lock is active when no index diff was detected

      rdy &= ~(lock.isSet() & lock.isAct());  // if any requested lock was not set or not active, we're not ready.
    }
    
    return rdy;

  }
