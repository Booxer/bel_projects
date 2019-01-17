#include "blocklock.h"

vEbwrs& BlockLock::lock(vEbwrs& ew) {
  ew.va  += qflagsAdr;
  std::cout << "setting locks: wrset " << (int)wr.set << " wrstat " << (int)wr.stat << " rdset " << (int)rd.set << " rdstat " << (int)rd.stat << std::endl;
  std::cout << "intended outcome: 0x " << std::hex << ((((uint32_t)wr.set | (uint32_t)wr.stat) << BLOCK_CMDQ_DNW_POS) | (((uint32_t)rd.set | (uint32_t)rd.stat) << BLOCK_CMDQ_DNR_POS)) << std::endl;
  writeLeNumberToBeBytes(ew.vb, ((((uint32_t)wr.set | (uint32_t)wr.stat) << BLOCK_CMDQ_DNW_POS) | (((uint32_t)rd.set | (uint32_t)rd.stat) << BLOCK_CMDQ_DNR_POS)) );
  ew.vcs += leadingOne(1);
  return ew;
}

vEbwrs& BlockLock::unlock(vEbwrs& ew ) {
  ew.va  += qflagsAdr;
  std::cout << "setting unlocks: wrclr " << (int)wr.set << " wrstat " << (int)wr.stat << " rdclr " << (int)rd.set << " rdstat " << (int)rd.stat << std::endl;
  std::cout << "intended outcome: 0x " << std::hex << ((((uint32_t)wr.stat & (uint32_t)~wr.clr) << BLOCK_CMDQ_DNW_POS) | (((uint32_t)rd.stat & (uint32_t)~rd.clr) << BLOCK_CMDQ_DNR_POS)) << std::endl;
  writeLeNumberToBeBytes(ew.vb, ((((uint32_t)wr.stat & (uint32_t)~wr.clr) << BLOCK_CMDQ_DNW_POS) | (((uint32_t)rd.stat & (uint32_t)~rd.clr) << BLOCK_CMDQ_DNR_POS)) );
  ew.vcs += leadingOne(1);
  return ew;
}

vEbrds& BlockLock::updateStat(vEbrds& er) {
  er.va  += qflagsAdr;
  er.vcs += leadingOne(1);
  return er;
}

vEbrds& BlockLock::updateAct(vEbrds& er) {
  vAdr tmp = {wridxAdr, rdidxAdr}; 
  er.va  += tmp;
  er.vcs += leadingOne(tmp.size());
  return er;
}

bool BlockLock::isAllSet()  const {
  //check if any lock bits is requested but not present. If so, return false.
  return ( ((wr.set & wr.stat) | ~wr.set) & ((rd.set & rd.stat) | ~rd.set) );
}

bool BlockLock::isAnySet() const {
  //check if any lock bits is requested but not present. If so, return false.
  return ( ((wr.set & wr.stat) | ~wr.set) & ((rd.set & rd.stat) | ~rd.set) );
}

bool BlockLock::isAct() const {
  //check if any lock bits is requested but not present. If so, return false.
  return ( ((wr.set & wr.act) | ~wr.set) & ((rd.set & rd.act) | ~rd.set) );
}

