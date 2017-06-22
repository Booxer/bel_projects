#ifndef _MEM_UNIT_H_
#define _MEM_UNIT_H_

#include <stdint.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <boost/optional.hpp>
#include <stdlib.h>
#include "common.h"
#include "graph.h"
#include "ftm_common.h"
#include "hashmap.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define MAX_IDX 32
#define IDX_BMPS (MAX_IDX / 32)





//typedef std::map< std::string, myData >::iterator itMap ;



  typedef struct  {
    vertex_t  v;
    uint32_t  hash;
    uint8_t   b[_MEM_BLOCK_SIZE];
  } parserMeta;

  typedef struct  {
    uint32_t  adr;
    uint32_t  hash;
    vertex_t  v;
    uint8_t   b[_MEM_BLOCK_SIZE];
    bool      transfer;
  } chunkMeta;


typedef std::map<std::string, chunkMeta>      aMap;
typedef std::map<uint32_t, parserMeta>        pMap;
typedef std::set<uint32_t>                    aPool; // contains all available addresses in LM32 memory area
typedef boost::container::vector<chunkMeta*>  vChunk;
typedef std::map<std::string, chunkMeta>      aMap;


class MemUnit {
  
  const uint8_t   cpu;
  const uint32_t  extBaseAdr;
  const uint32_t  intBaseAdr;
  const uint32_t  sharedOffs;
  const uint32_t  poolSize;
  const uint32_t  bmpLen;
  const uint32_t  startOffs; // baseAddress + bmpLen rounded up to next multiple of MEM_BLOCK_SIZE to accomodate BMP
  const uint32_t  endOffs;   // baseAddress + poolSize rounded down to next multiple of MEM_BLOCK_SIZE, can only use whole blocks 
  Graph  gUp;
  HashMap& hashMap;
  Graph gDown;
  vBuf   uploadBmp;
  vBuf   downloadBmp;

  aPool  memPool;
  aMap   allocMap;
  pMap   parserMap;
  
  
  


  

public:  




  MemUnit(uint8_t cpu, uint32_t extBaseAdr, uint32_t intBaseAdr, uint32_t sharedOffs, uint32_t poolSize, HashMap& hm) 
        : cpu(cpu), extBaseAdr(extBaseAdr), intBaseAdr(intBaseAdr), sharedOffs(sharedOffs),
          poolSize(poolSize), bmpLen( poolSize / _MEM_BLOCK_SIZE), 
          startOffs(sharedOffs + ((((bmpLen + 8 -1)/8 + _MEM_BLOCK_SIZE -1) / _MEM_BLOCK_SIZE) * _MEM_BLOCK_SIZE)),
          endOffs(sharedOffs + ((poolSize / _MEM_BLOCK_SIZE) * _MEM_BLOCK_SIZE)),
          hashMap(hm),
          uploadBmp(vBuf( ((((bmpLen + 8 -1)/8 + _MEM_BLOCK_SIZE -1) / _MEM_BLOCK_SIZE) * _MEM_BLOCK_SIZE) )), 
          downloadBmp(vBuf( ((((bmpLen + 8 -1)/8 + _MEM_BLOCK_SIZE -1) / _MEM_BLOCK_SIZE) * _MEM_BLOCK_SIZE) )) { 
            initMemPool();
          }
  ~MemUnit() { };

  Graph& getUpGraph() {return gUp;}
  Graph& getDownGraph() {return gDown;}

  //MemPool Functions
  void banChunk(uint32_t adr) {memPool.erase(adr);};
  void initMemPool();
  bool acquireChunk(uint32_t &adr);
  bool freeChunk(uint32_t &adr);

  //Management functions

  uint32_t getFreeChunkQty() { return memPool.size(); }
  uint32_t getFreeSpace() { return memPool.size() * _MEM_BLOCK_SIZE; }
  uint32_t getUsedSpace() { return poolSize - (memPool.size() * _MEM_BLOCK_SIZE); }
  
  //Allocation functions
  bool allocate(const std::string& name, vertex_t v);
  bool insert(const std::string& name, uint32_t adr);
  bool deallocate(const std::string& name);
  chunkMeta* lookupName(const std::string& name) const;
  parserMeta* lookupAdr(uint32_t adr) const;
  vChunk getAllChunks() const;



  

  //Addr Functions
  const uint32_t extAdr2adr(const uint32_t ea)    const  { return (ea == LM32_NULL_PTR ? LM32_NULL_PTR : ea - extBaseAdr); }
  const uint32_t intAdr2adr(const uint32_t ia)    const  { return (ia == LM32_NULL_PTR ? LM32_NULL_PTR : ia - intBaseAdr); }
  const uint32_t extAdr2intAdr(const uint32_t ea) const  { return (ea == LM32_NULL_PTR ? LM32_NULL_PTR : ea - extBaseAdr + intBaseAdr); }
  const uint32_t intAdr2extAdr(const uint32_t ia) const  { return (ia == LM32_NULL_PTR ? LM32_NULL_PTR : ia - intBaseAdr + extBaseAdr); }
  const uint32_t adr2extAdr(const uint32_t a)     const  { return a + extBaseAdr; }
  const uint32_t adr2intAdr(const uint32_t a)     const  { return a + intBaseAdr; }
  
  //Upload Functions
  void prepareUpload(Graph& g);

  void createUploadBmp();
  vAdr getUploadAdrs() const;
  vBuf getUploadData();

  //Download Functions
  void setDownloadBmp(vBuf dlBmp) {downloadBmp = dlBmp;} // vHexDump ("DLBMP", downloadBmp, downloadBmp.size());}
  const vAdr getDownloadBMPAdrs() const;
  const vAdr getDownloadAdrs() const;
  void parseDownloadData(vBuf downloadData);


  void showUp(const std::string& title, const std::string& logDictFile );

  

};



#endif
