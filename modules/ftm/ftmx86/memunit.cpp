#include "memunit.h"
#include "common.h"


  void MemUnit::updatememPoolFromBmp() {
    //remember not to put the BMP into the pool of available memory

    
    
  } 

  void MemUnit::initMemPool() { 
    memPool.clear();
    std::cout << "BaseAdr = " << baseAdr << ", Poolsize = " << poolSize << ", bmpLen = " << bmpLen << ", startOffs = " << startOffs << ", endOffs = " << endOffs << ", vBufSize = " << mgmtBmp.size() << std::endl;
    for(uint32_t adr = baseAdr + startOffs; adr < baseAdr + endOffs; adr += MEM_BLOCK_SIZE) { 
      //Never issue <baseAddress - (baseAddress + bmpLen -1) >, as this is where Mgmt bitmap vector resides     
      //std::cout << std::hex << adr << std::endl; 
      memPool.insert(adr); 
    }
  }

  bool MemUnit::acquireChunk(uint32_t &adr) {
    bool ret = true;
    if ( memPool.empty() ) {
      ret = false;
    } else {
      adr = *(memPool.begin());
      memPool.erase(adr);
    }
    return ret;
  }


  bool MemUnit::freeChunk(uint32_t &adr) {
    bool ret = true;
    if ((adr % MEM_BLOCK_SIZE) || (memPool.count(adr) > 0))  {ret = false;} //unaligned or attempted double entry, throw exception
    else memPool.insert(adr);
    return ret;
  }        

  void MemUnit::updateBmpFromAlloc() {
    for (itBuf it = mgmtBmp.begin(); it < mgmtBmp.end(); it++) {
      (*it) = 0;
    }    

    //Go through allocmap and update Bmp
    for (itAm it = allocMap.begin(); it != allocMap.end(); it++) {
      if( (it->second.adr >= baseAdr + startOffs) && (it->second.adr < baseAdr + endOffs)) {
        int bitIdx = (it->second.adr - baseAdr) / MEM_BLOCK_SIZE;
        uint8_t tmp = 1 << (bitIdx % 8);
        printf("Bidx = %u, bufIdx = %u, val = %x\n", bitIdx, bitIdx / 8 , tmp);
        
        mgmtBmp[bitIdx / 8] |= tmp;
      } else {//something's awfully wrong, address out of scope!
        std::cout << "Address 0x" << std::hex << it->second.adr << " is not within 0x" << std::hex << baseAdr + startOffs << "-" << std::hex << baseAdr + endOffs << std::endl;
      }
    }
    
  }
/*
  void MemUnit::downLoadChunks() {
    //get BMP

    //create Address vector
    aPool downloadAdr;
    uint32_t adr;

    for (itBuf it = mgmtBmp.begin(); it < mgmtBmp.end(); it++) {
      for (int i=0; i <8; i++) {
        if ((*it) & (1<<i)) {
          adr = baseAdr + int(mgmtBmp.end() - it)*8 + i * MEM_BLOCK_SIZE;
          downloadAdr.push_back(adr);
          std::cout << "Found Address 0x" << std::hex << adr << std::endl;
        }
      }
    }    

    vBuf tmp = vBuf(MEM_BLOCK_SIZE);
    //while downloadAdr
      //download Chunk via eb into tmp vec

      //get hash

      //lookup Name

      //allocate and copy tmp    

    
    
  }
*/



  /*
void MemUnit::prepareUpload() {

      //check allocation
      //go through graph
      //call allocate    

    //update BMP



    //serialise
      //go through graph
      //call serialise   
  }


  void MemUnit::upload() {
      

    //split all allocmap elements marked for upload (transfer = true) into network packets ( div 38)

      //play each entry's buffer as eb operations
      //send cycle

    //play BMP as eb operations
    //send cycle

    
    
  }
*/


  //Allocation functions
  bool MemUnit::allocate(const std::string& name) {
    uint32_t chunkAdr, hash;
    bool ret = true;

    insertHash(name, hash);
    if ( (allocMap.count(name) == 0) && acquireChunk(chunkAdr) ) { allocMap[name] = (chunkMeta) {chunkAdr, hash, vBuf(32)};  

    } else {ret = false;}
    return ret;
  }

  bool MemUnit::insert(const std::string& name, uint32_t adr) {}

  bool MemUnit::deallocate(const std::string& name) {
    bool ret = true;
    if ( (allocMap.count(name) > 0) && freeChunk(allocMap[name].adr) ) { allocMap.erase(name); 
    } else {ret = false;}
    return ret;
  }

  bool MemUnit::lookupName2Chunk(const std::string& name, chunkMeta*& chunk) {
    bool ret = true;
    if (allocMap.count(name) > 0) { chunk = (chunkMeta*)&(allocMap[name]); 
    } else {ret = false;}
    return ret;
  }

  //Hash functions

  bool MemUnit::insertHash(const std::string& name, uint32_t &hash) {
    hash = FnvHash::fnvHash(name.c_str());

    if (hashMap.count(hash) > 0) return false;
    else hashMap[hash] = name;
    return true;
  }

  bool MemUnit::removeHash(const uint32_t hash) {
    if (hashMap.count(hash) > 0) {hashMap.erase(hash); return true;}
    return false;
  }
  
  bool MemUnit::lookupHash2Name(const uint32_t hash, std::string& name)  {
    if (hashMap.count(hash) > 0) {name = hashMap.at(hash); return true;}
    return false;
  }


