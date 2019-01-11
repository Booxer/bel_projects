void EbWrapper::simAdrTranslation (uint32_t a, uint8_t& cpu, uint32_t& arIdx) {
  //get cpu
  /*
   if (debug) sLog << "cpuQty "  << getCpuQty() << std::endl;
  for (uint8_t cpuIdx = 0; cpuIdx < getCpuQty(); cpuIdx++) {
    if (debug) sLog << "a : " << std::hex << a << ", cmpA " << simRamAdrMap[cpuIdx] << " cpu " << (int)cpuIdx << " arIdx "  << arIdx << std::endl;
    if (simRamAdrMap[cpuIdx] > a) break;
    cpu = cpuIdx;
  }
  */
  cpu = ((a >> 17) & 0x7) -1;
  arIdx = atDown.adrConv(AdrType::EXT, AdrType::MGMT, cpu, a) >> 2;
}

void EbWrapper::simRamWrite (uint32_t a, eb_data_t d) {
  uint8_t cpu = -1;
  uint32_t arIdx;
  if (debug) sLog << "cpu : " << (int)cpu << " arIdx " << std::hex << arIdx << std::endl;
  simAdrTranslation (a, cpu, arIdx);
  simRam[cpu][arIdx] = d;
}


void EbWrapper::simRamRead (uint32_t a, eb_data_t* d) {
  uint8_t cpu;
  uint32_t arIdx;
  simAdrTranslation (a, cpu, arIdx);
  *d = simRam[cpu][arIdx];
}

int EbWrapper::simWriteCycle(vAdr va, vBuf& vb) {
  if (debug) sLog << "Starting Write Cycle" << std::endl;
  eb_data_t veb[va.size()];

  for(int i = 0; i < (va.end()-va.begin()); i++) {
   uint32_t data = vb[i*4 + 0] << 24 | vb[i*4 + 1] << 16 | vb[i*4 + 2] << 8 | vb[i*4 + 3];
   veb[i] = (eb_data_t)data;
  }


  for(int i = 0; i < (va.end()-va.begin()); i++) {

    if (debug) sLog << " Writing @ 0x" << std::hex << std::setfill('0') << std::setw(8) << va[i] << " : 0x" << std::hex << std::setfill('0') << std::setw(8) << veb[i] << std::endl;
    simRamWrite(va[i], veb[i]);
  }


  return 0;

}





vBuf EbWrapper::simReadCycle(vAdr va)
{


  eb_data_t veb[va.size()];
  vBuf ret = vBuf(va.size() * 4);
  if (debug) sLog << "Starting Read Cycle" << std::endl;
  //sLog << "Got Adr Vec with " << va.size() << " Adrs" << std::endl;


  for(int i = 0; i < (va.end()-va.begin()); i++) {
    if (debug) sLog << " Reading @ 0x" << std::hex << std::setfill('0') << std::setw(8) << va[i] << std::endl;
    simRamRead(va[i], (eb_data_t*)&veb[i]);
  }

  //FIXME use endian functions
  for(unsigned int i = 0; i < va.size(); i++) {
    ret[i * 4]     = (uint8_t)(veb[i] >> 24);
    ret[i * 4 + 1] = (uint8_t)(veb[i] >> 16);
    ret[i * 4 + 2] = (uint8_t)(veb[i] >> 8);
    ret[i * 4 + 3] = (uint8_t)(veb[i] >> 0);
  }

  return ret;
}



int EbWrapper::ebWriteCycle(vAdr va, vBuf& vb, vBl vcs)
{

  //eb_status_t status;
  //FIXME What about MTU? What about returned eb status ??

  if (debug) sLog << "Starting Write Cycle" << std::endl;
  Cycle cyc;
  eb_data_t veb[va.size()];

  for(int i = 0; i < (va.end()-va.begin()); i++) {
   uint32_t data = vb[i*4 + 0] << 24 | vb[i*4 + 1] << 16 | vb[i*4 + 2] << 8 | vb[i*4 + 3];
   veb[i] = (eb_data_t)data;
  }
  try {
    cyc.open(dev);
    for(int i = 0; i < (va.end()-va.begin()); i++) {
    if (i && vcs.at(i)) {
      cyc.close();
      if (debug) sLog << "Close and open next Write Cycle" << std::endl;
      cyc.open(dev);
    }

    if (debug) sLog << " Writing @ 0x" << std::hex << std::setfill('0') << std::setw(8) << va[i] << " : 0x" << std::hex << std::setfill('0') << std::setw(8) << veb[i] << std::endl;
    cyc.write(va[i], EB_BIG_ENDIAN | EB_DATA32, veb[i]);

    }
    cyc.close();
  } catch (etherbone::exception_t const& ex) {
    throw std::runtime_error("Etherbone " + std::string(ex.method) + " returned " + std::string(eb_status(ex.status)) + "\n" );
  }

   return 0;
}

int   EbWrapper::ebWriteCycle(vAdr va, vBuf& vb) {return  ebWriteCycle(dev, va, vb, leadingOne(va.size()));}



vBuf EbWrapper::ebReadCycle(vAdr va, vBl vcs)
{
  //FIXME What about MTU? What about returned eb status ??

  Cycle cyc;
  eb_data_t veb[va.size()];
  vBuf ret = vBuf(va.size() * 4);
  if (debug) sLog << "Starting Read Cycle" << std::endl;
  //sLog << "Got Adr Vec with " << va.size() << " Adrs" << std::endl;

  try {
    cyc.open(dev);
    for(int i = 0; i < (va.end()-va.begin()); i++) {
    //FIXME dirty break into cycles
    if (i && vcs.at(i)) {
      cyc.close();
      if (debug) sLog << "Close and open next Read Cycle" << std::endl;
      cyc.open(dev);
    }
    if (debug) sLog << " Reading @ 0x" << std::hex << std::setfill('0') << std::setw(8) << va[i] << std::endl;
    cyc.read(va[i], EB_BIG_ENDIAN | EB_DATA32, (eb_data_t*)&veb[i]);
    }
    cyc.close();

  } catch (etherbone::exception_t const& ex) {
    throw std::runtime_error("Etherbone " + std::string(ex.method) + " returned " + std::string(eb_status(ex.status)) + "\n" );
  }
  //FIXME use endian functions
  for(unsigned int i = 0; i < va.size(); i++) {
    ret[i * 4]     = (uint8_t)(veb[i] >> 24);
    ret[i * 4 + 1] = (uint8_t)(veb[i] >> 16);
    ret[i * 4 + 2] = (uint8_t)(veb[i] >> 8);
    ret[i * 4 + 3] = (uint8_t)(veb[i] >> 0);
  }

  return ret;
}

vBuf EbWrapper::ebReadCycle(vAdr va) {return  ebReadCycle(dev, va, leadingOne(va.size()));}

int EbWrapper::ebWriteWord(uint32_t adr, uint32_t data)
{
  uint8_t b[_32b_SIZE_];
  writeLeNumberToBeBytes(b, data);
  vAdr vA({adr});
  vBuf vD(std::begin(b), std::end(b) );

  return ebWriteCycle(ebd, vA, vD);
}

uint32_t EbWrapper::ebReadWord(uint32_t adr)
{
  vAdr vA({adr});
  vBuf vD = ebReadCycle(ebd, vA);
  uint8_t* b = &vD[0];

  return writeBeBytesToLeNumber<uint32_t>(b);
}

 //Reads and returns a 64 bit word from DM
uint64_t EbWrapper::read64b(uint32_t startAdr) {
  vAdr vA({startAdr + 0, startAdr + _32b_SIZE_});
  vBuf vD = ebReadCycle(ebd, vA);
  uint8_t* b = &vD[0];

  return writeBeBytesToLeNumber<uint64_t>(b);
}

int EbWrapper::write64b(uint32_t startAdr, uint64_t d) {
  uint8_t b[_TS_SIZE_];
  writeLeNumberToBeBytes(b, d);
  vAdr vA({startAdr + 0, startAdr + _32b_SIZE_});
  vBuf vD(std::begin(b), std::end(b) );

  return ebWriteCycle(ebd, vA, vD);

}

bool EbWrapper::simConnect() {
    simRam.clear();
    simRamAdrMap.clear();
 
    ebdevname = "simDummy"; //copy to avoid mem trouble later
    uint8_t mappedIdx = 0;
    uint32_t const intBaseAdr   = 0x1000000;
    uint32_t const sharedSize   = 98304;
    uint32_t const rawSize      = 131072;
    uint32_t const sharedOffs   = 0x500;
    uint32_t const devBaseAdr   = 0x4120000;
    cpuQty = 4;

    atUp.clear();
    atUp.removeMemories();
    gUp.clear();
    atDown.clear();
    atDown.removeMemories();
    gDown.clear();
    cpuIdxMap.clear();
    cpuDevs.clear();




    sLog << "Connecting to Sim... ";
    simRam.reserve(cpuQty);

    for(int cpuIdx = 0; cpuIdx< cpuQty; cpuIdx++) {
      simRam[cpuIdx]        = new uint32_t [(rawSize + _32b_SIZE_ -1) >> 2];
      cpuIdxMap[cpuIdx]     = mappedIdx;
      uint32_t extBaseAdr   = devBaseAdr + cpuIdx * rawSize;
      simRamAdrMap[cpuIdx]  = extBaseAdr;
      uint32_t peerBaseAdr  = WORLD_BASE_ADR  + extBaseAdr;
      uint32_t space        = sharedSize - _SHCTL_END_;

      atUp.addMemory(cpuIdx, extBaseAdr, intBaseAdr, peerBaseAdr, sharedOffs, space, rawSize );
      atDown.addMemory(cpuIdx, extBaseAdr, intBaseAdr, peerBaseAdr, sharedOffs, space, rawSize );
      mappedIdx++;
    }
    sLog << "done" << std::endl;
    return true;

}


bool EbWrapper::ebConnect(const std::string& en, bool test) {
    testmode = test;


    ebdevname = std::string(en); //copy to avoid mem trouble later
    bool  ret = false;
    uint8_t mappedIdx = 0;
    int expVersion = parseFwVersionString(EXP_VER), foundVersion;
    int foundVersionMax = -1;

    if (expVersion <= 0) {throw std::runtime_error("Bad required minimum firmware version string received from Makefile"); return false;}

    atUp.clear();
    atUp.removeMemories();
    gUp.clear();
    atDown.clear();
    atDown.removeMemories();
    gDown.clear();
    cpuIdxMap.clear();
    cpuDevs.clear();

    vFw.clear();

    if(verbose) sLog << "Connecting to " << en << "... ";
    try {
      ebs.open(0, EB_DATAX|EB_ADDRX);
      ebd.open(ebs, ebdevname.c_str(), EB_DATAX|EB_ADDRX, 3);
      ebd.sdb_find_by_identity(CluTime::vendID, CluTime::devID, cluTimeDevs);
      if (cluTimeDevs.size() < 1) throw std::runtime_error("Could not find Cluster Time Module on DM (needed for WR time). Something is wrong\n");

      ebd.sdb_find_by_identity(SDB_VENDOR_GSI,SDB_DEVICE_DIAG, diagDevs);

      ebd.sdb_find_by_identity(SDB_VENDOR_GSI,SDB_DEVICE_LM32_RAM, cpuDevs);
      if (cpuDevs.size() >= 1) {
        cpuQty = cpuDevs.size();


        for(int cpuIdx = 0; cpuIdx< cpuQty; cpuIdx++) {
          //only create MemUnits for valid DM CPUs, generate Mapping so we can still use the cpuIdx supplied by User
          const std::string fwIdROM = getFwIdROM(cpuIdx);
          foundVersion = getFwVersion(fwIdROM);
          foundVersionMax = foundVersionMax < foundVersion ? foundVersion : foundVersionMax;
          vFw.push_back(foundVersion);
          int expVersionMin = expVersion;
          int expVersionMax = (expVersion / (int)FwId::VERSION_MAJOR_MUL) * (int)FwId::VERSION_MAJOR_MUL
                             + 99 * (int)FwId::VERSION_MINOR_MUL
                             + 99 * (int)FwId::VERSION_REVISION_MUL;

          if ( (foundVersion >= expVersionMin) && (foundVersion <= expVersionMax) ) {
            //FIXME check for consequent use of cpu index map!!! I'm sure there'll be absolute chaos throughout the lib if CPUs indices were not continuous
            cpuIdxMap[cpuIdx]    = mappedIdx;

            uint32_t extBaseAdr   = cpuDevs[cpuIdx].sdb_component.addr_first;
            uint32_t intBaseAdr   = getIntBaseAdr(fwIdROM);
            uint32_t peerBaseAdr  = WORLD_BASE_ADR + extBaseAdr;
            uint32_t rawSize      = cpuDevs[cpuIdx].sdb_component.addr_last - cpuDevs[cpuIdx].sdb_component.addr_first;
            uint32_t sharedOffs   = getSharedOffs(fwIdROM);
            uint32_t space        = getSharedSize(fwIdROM) - _SHCTL_END_;

              atUp.addMemory(cpuIdx, extBaseAdr, intBaseAdr, peerBaseAdr, sharedOffs, space, rawSize );
            atDown.addMemory(cpuIdx, extBaseAdr, intBaseAdr, peerBaseAdr, sharedOffs, space, rawSize );
            mappedIdx++;
          }
          /*
          sLog << "#" << (int)cpuIdx << " Shared Offset 0x" << std::hex <<  atDown.getMemories()[cpuIdx].sharedOffs << std::endl;
          sLog << "#" << (int)cpuIdx << " BmpSize 0x" << std::hex <<  atDown.getMemories()[cpuIdx].bmpSize << std::endl;
          sLog << "#" << (int)cpuIdx << " SHCTL 0x" << std::hex <<  _SHCTL_END_ << std::endl;
          sLog << "#" << (int)cpuIdx << " Start Offset 0x" << std::hex <<  atDown.getMemories()[cpuIdx].startOffs << std::endl;
          */
        }
        ret = true;
      }
    } catch (etherbone::exception_t const& ex) {
      throw std::runtime_error("Etherbone " + std::string(ex.method) + " returned " + std::string(eb_status(ex.status)) + "\n" );
    } catch(...) {
      throw;// std::runtime_error("Could not find CPUs running valid DM Firmware\n" );
    }

    if(verbose) {
      sLog << " Done."  << std::endl << "Found " << getCpuQty() << " Cores, " << cpuIdxMap.size() << " of them run a valid DM firmware." << std::endl;
    }
    std::string fwCause = foundVersionMax == -1 ? "" : "Requires FW v" + createFwVersionString(expVersion) + ", found " + createFwVersionString(foundVersionMax);
    if (cpuIdxMap.size() == 0) {throw std::runtime_error("No CPUs running a valid DM firmware found. " + fwCause);}


    return ret;

  }

  bool EbWrapper::simDisconnect() {


    bool ret = false;

    if(verbose) sLog << "Disconnecting ... ";
    if(verbose) sLog << " Done" << std::endl;
    
    return ret;
  }

  bool EbWrapper::ebDisconnect() {
    bool ret = false;

    if(verbose) sLog << "Disconnecting ... ";
    try {
      ebd.close();
      ebs.close();
      cpuQty = -1;
      ret = true;
    } catch (etherbone::exception_t const& ex) {
      throw std::runtime_error("Etherbone " + std::string(ex.method) + " returned " + std::string(eb_status(ex.status)) + "\n" );
      //TODO report why we could not disconnect
    }
    
    if(verbose) sLog << " Done" << std::endl;
    
    return ret;
  }


  readCycle () {
    if (va.size() != vcs.size()) throw std::runtime_error(" EB Read cycle Adr / Flow control vector lengths (" + std::to_string(va.size()) + "/" + std::to_string(vcs.size()) + ") do not match\n");

    if (sim) simWriteCycle();
    else ebWriteCycle();

  writeCycle()  
    if ( (va.size() != vcs.size()) || ( va.size() * _32b_SIZE_ != vb.size() ) )
    throw std::runtime_error(" EB/sim write cycle Adr / Data / Flow control vector lengths (" + std::to_string(va.size()) + "/" + std::to_string(vb.size() /  _32b_SIZE_) + "/" + std::to_string(vcs.size()) +") do not match\n");

    if (sim) simWriteCycle();
    else ebWriteCycle();

  }