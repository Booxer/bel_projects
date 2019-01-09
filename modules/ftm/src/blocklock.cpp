

vEbwrs& BlockLock::lock(vEbwrs& ew) {
  ew.va  += qflagsAdr;
  ew.vb  += ((wr.set | wr.stat) << BLOCK_CMDQ_DNW_POS) | ((rd.set | rd.stat) << BLOCK_CMDQ_DNR_POS);
  ew.vcs += leadingOne(1);
  return ew;
}


vEbwrs& BlockLock::unlock(vEbwrs& ew ) {
  ew.va  += qflagsAdr;
  ew.vb  += ((wr.stat & ~wr.clr) << BLOCK_CMDQ_DNW_POS) | ((rd.stat & ~rd.clr) << BLOCK_CMDQ_DNR_POS);
  ew.vcs += leadingOne(1);
  return ew;
}

bool BlockLock::isLockSet() {
  //check if any lock bits is requested but not present. If so, return false.
  return ( (wr.set & wr.stat | ~wr.set) & (rd.set & rd.stat | ~rd.set) );
}

bool BlockLock::isLockAct() {
  //check if any lock bits is requested but not present. If so, return false.
  return ( (wr.set & wr.act | ~wr.set) & (rd.set & rd.act | ~rd.set) );
}

vEbrds& BlockLock::updateStat(vEbrds& er) {
  ew.va  += qflagsAdr;
  ew.vcs += leadingOne(1);
  return er;
}


vEbrds& BlockLock::updateStatAndRdy(vEbrds& er) {
  vAdr tmp = {qflagsAdr, wridxAdr, rdidxAdr, wridxAdr, rdidxAdr}; 
  ew.va  += tmp;
  ew.vcs += leadingOne(tmp.size());
  return er;
}



