#include <boost/shared_ptr.hpp>
#include <algorithm>  
#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <boost/algorithm/string.hpp>

#include "common.h"
#include "propwrite.h"
#include "graph.h"
#include "carpeDM.h"
#include "visitoruploadcrawler.h"
#include "visitordownloadcrawler.h"
#include "dotstr.h"
#include "idformat.h"

namespace dnt = DotStr::Node::TypeVal;


  //Generate download Bmp addresses. For downloads, this has to be two pass: get bmps first, then use them to get the node locations to read 
  vEbrds CarpeDM::gatherDownloadBmpVector() {
    //sLog << "Starting download bmp address vectors" << std::endl;
    AllocTable& at = atDown;
    vEbrds er;

     //add all Bmp addresses to return vector
    for(unsigned int i = 0; i < at.getMemories().size(); i++) {
      //generate addresses of Bmp's address range
      for (uint32_t adr = at.adrConv(AdrType::MGMT, AdrType::EXT,i, at.getMemories()[i].bmpOffs); adr < at.adrConv(AdrType::MGMT, AdrType::EXT,i, at.getMemories()[i].startOffs); adr += _32b_SIZE_) {
        er.va.push_back(adr);
        er.vcs.push_back(adr == at.adrConv(AdrType::MGMT, AdrType::EXT,i, at.getMemories()[i].bmpOffs));
      }  
    }
    return er;
  }

    

  vEbrds CarpeDM::gatherDownloadDataVector() {
    //sLog << "Starting download bmp data vectors" << std::endl;
    AllocTable& at = atDown;
    vEbrds er;
    //go through Memories
    for(unsigned int i = 0; i < at.getMemories().size(); i++) {
      //go through a memory's bmp bits, starting at number of nodes the bmp itself needs (bmpSize / memblocksize). Otherwise, we'd needlessly download the bmp again

      //sLog << "Bmp" << i << ":" << std::endl;
      for(unsigned int bitIdx = at.getMemories()[i].bmpSize / _MEM_BLOCK_SIZE; bitIdx < at.getMemories()[i].bmpBits; bitIdx++) {
        //if the bit says the node is used, we add the node to read addresses
        //sLog << "Bit Idx " << bitIdx << " valid " << at.getMemories()[i].getBmpBit(bitIdx) << " na 0x" << std::hex << at.getMemories()[i].sharedOffs + bitIdx * _MEM_BLOCK_SIZE << std::endl;
        if (at.getMemories()[i].getBmpBit(bitIdx)) {
          uint32_t nodeAdr = at.getMemories()[i].bmpOffs + bitIdx * _MEM_BLOCK_SIZE;
           //generate addresses of node's address range
          for (uint32_t adr = at.adrConv(AdrType::MGMT, AdrType::EXT,i, nodeAdr); adr < at.adrConv(AdrType::MGMT, AdrType::EXT,i, nodeAdr + _MEM_BLOCK_SIZE); adr += _32b_SIZE_ ) {
            er.va.push_back(adr);
            er.vcs.push_back(adr == at.adrConv(AdrType::MGMT, AdrType::EXT,i, nodeAdr));
          }
        }
      }      
    }
    return er;
  }  
    
   void CarpeDM::parseDownloadMgmt(const vBuf& downloadData) {
    AllocTable& at = atDown;
    

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //create MgmtTable
    //sLog << std::dec << "dl size " << downloadData.size() << std::endl;
    uint32_t found = 0;

    uint32_t nodeCnt = 0;
    //go through Memories
    for(unsigned int i = 0; i < at.getMemories().size(); i++) {
      //go through Bmp
      for(unsigned int bitIdx = at.getMemories()[i].bmpSize / _MEM_BLOCK_SIZE; bitIdx < at.getMemories()[i].bmpBits; bitIdx++) {
        if (at.getMemories()[i].getBmpBit(bitIdx)) {
          
          uint32_t    localAdr  = nodeCnt * _MEM_BLOCK_SIZE; nodeCnt++;
          uint32_t    flags     = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&downloadData[localAdr + NODE_FLAGS]); 
          uint32_t    type      = (flags >> NFLG_TYPE_POS) & NFLG_TYPE_MSK;
          if (type != NODE_TYPE_MGMT) continue; // skip all non mgmt nodes
          uint8_t     cpu       = i;
          uint32_t    adr       = at.getMemories()[i].bmpOffs + bitIdx * _MEM_BLOCK_SIZE;

          //we need to conform to the allocation rules and register our management nodes
          if (!(at.insertMgmt(cpu, adr, (uint8_t*)&downloadData[localAdr]))) {throw std::runtime_error( std::string("Address collision when adding mgmt node at "));};
          found++;
        }
      }
    }

    sLog << "Mgmt found " << found << " data chunks. Trying to recover GroupTable ..." << std::endl;

    // recover container
    vBuf tmpMgmtRecovery = decompress(atDown.recoverMgmt());


    // Rebuild Grouptable
    GroupTable gtTmp;
    std::string tmpStr = std::string(tmpMgmtRecovery.begin(), tmpMgmtRecovery.end());
    sLog << "Bytes expected: " << atDown.getMgmtLLsize() << ", recovered: " << found << std::endl << std::endl;
    sLog << tmpStr << std::endl;
    if (tmpStr.size()) gtTmp.load(tmpStr); 
    gt = gtTmp;
    // Rebuild HashMap from Grouptable
    hm.clear();
    for(auto& it : gt.getTable()) {
      hm.add(it.node);
    }
    // clean up
    atDown.deallocateAllMgmt();
    //atDown.updateBmps(); NOT ALLOWED: the nodes were downloaded in sequences, we cannot skip any during processing!
    //pool and bmp diverge from here on, but for the greater good.

  } 



  void CarpeDM::parseDownloadData(const vBuf& downloadData) {
    Graph& g = gDown;
    AllocTable& at = atDown;
    std::stringstream stream;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //create AllocTable and Vertices
    //sLog << std::dec << "dl size " << downloadData.size() << std::endl;

    uint32_t nodeCnt = 0;
    //go through Memories
    for(unsigned int i = 0; i < at.getMemories().size(); i++) {
      //go through Bmp
      for(unsigned int bitIdx = at.getMemories()[i].bmpSize / _MEM_BLOCK_SIZE; bitIdx < at.getMemories()[i].bmpBits; bitIdx++) {
        if (at.getMemories()[i].getBmpBit(bitIdx)) {
          
          uint32_t    localAdr  = nodeCnt * _MEM_BLOCK_SIZE; nodeCnt++;
          uint32_t    adr       = at.getMemories()[i].bmpOffs + bitIdx * _MEM_BLOCK_SIZE;
          //sLog << "THE adr : 0x" << std::hex << adr << std::endl; 
          uint32_t    hash      = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&downloadData[localAdr + NODE_HASH]);
          uint32_t    flags     = writeBeBytesToLeNumber<uint32_t>((uint8_t*)&downloadData[localAdr + NODE_FLAGS]); //FIXME what about future requests to hashmap if we improvised the name from hash? those will fail ...
          uint32_t    type      = (flags >> NFLG_TYPE_POS) & NFLG_TYPE_MSK;
          uint8_t     cpu       = i;

          // IMPORTANT: skip all mgmt nodes
          if (type == NODE_TYPE_MGMT) continue; 

          stream.str(""); stream.clear();
          stream << "0x" << std::setfill ('0') << std::setw(sizeof(uint32_t)*2) << std::hex << hash;
          std::string name      = hm.lookup(hash) ? hm.lookup(hash).get() : DotStr::Misc::sHashType + stream.str();
          auto xPat  = gt.getTable().get<Groups::Node>().equal_range(name);
          std::string pattern   = (xPat.first != xPat.second ? xPat.first->pattern : DotStr::Misc::sUndefined);    
          auto xBp  = gt.getTable().get<Groups::Node>().equal_range(name);
          std::string beamproc  = (xBp.first != xBp.second ? xPat.first->beamproc : DotStr::Misc::sUndefined);
          
          //Vertex needs flags as a std::string. Convert to hex
          stream.str(""); stream.clear();
          stream << "0x" << std::setfill ('0') << std::setw(sizeof(uint32_t)*2) << std::hex << flags;
          std::string tmp(stream.str());

          //Add Vertex
          
          vertex_t v        = boost::add_vertex(myVertex(name, pattern, beamproc, std::to_string(cpu), hash, nullptr, "", tmp), g);
          //FIXME workaround for groupstable updates from download. not  nice ...
          
          g[v].bpEntry  = std::to_string((bool)(flags & NFLG_BP_ENTRY_LM32_SMSK));
          g[v].bpExit   = std::to_string((bool)(flags & NFLG_BP_EXIT_LM32_SMSK));
          g[v].patEntry = std::to_string((bool)(flags & NFLG_PAT_ENTRY_LM32_SMSK));
          g[v].patExit  = std::to_string((bool)(flags & NFLG_PAT_EXIT_LM32_SMSK));
          
          //std::cout << "atdown cpu " << (int)cpu << " Adr: 0x" << std::hex << adr <<  " Hash 0x" << hash << std::endl;
          //Add allocTable Entry
          //vBuf test(downloadData.begin() + localAdr, downloadData.begin() + localAdr + _MEM_BLOCK_SIZE);
          //vHexDump("TEST ****", test);
         
          if (!(at.insert(cpu, adr, hash, v, false))) {
            sLog << "Offending Node at: CPU " << (int)cpu << " 0x" << std::hex << adr << std::endl;
            hexDump("Dump", (char*)&downloadData[localAdr], _MEM_BLOCK_SIZE );
            throw std::runtime_error( std::string("Hash or address collision when adding node ") + name);
          };

          // Create node object for Vertex
          auto src = downloadData.begin() + localAdr;

          auto it  = at.lookupAdr(cpu, adr);
          if (!(at.isOk(it))) {throw std::runtime_error( std::string("Node at (dec) ") + std::to_string(adr) + std::string(", hash (dec) ") + std::to_string(hash) + std::string("not found. This is weird")); return;}
          
          auto* x = (AllocMeta*)&(*it);

          std::copy(src, src + _MEM_BLOCK_SIZE, (uint8_t*)&(x->b[0]));
        
          switch(type) {
            case NODE_TYPE_TMSG         : g[v].np = (node_ptr) new  TimingMsg(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sTMsg;       g[v].np->deserialise(); break;
            case NODE_TYPE_CNOOP        : g[v].np = (node_ptr) new       Noop(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sCmdNoop;    g[v].np->deserialise(); break;
            case NODE_TYPE_CFLOW        : g[v].np = (node_ptr) new       Flow(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sCmdFlow;    g[v].np->deserialise(); break;
            case NODE_TYPE_CFLUSH       : g[v].np = (node_ptr) new      Flush(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sCmdFlush;   g[v].np->deserialise(); break;
            case NODE_TYPE_CWAIT        : g[v].np = (node_ptr) new       Wait(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sCmdWait;    g[v].np->deserialise(); break;
            case NODE_TYPE_BLOCK_FIXED  : g[v].np = (node_ptr) new BlockFixed(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sBlockFixed; g[v].np->deserialise(); break;
            case NODE_TYPE_BLOCK_ALIGN  : g[v].np = (node_ptr) new BlockAlign(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sBlockAlign; g[v].np->deserialise(); break;
            case NODE_TYPE_QUEUE        : g[v].np = (node_ptr) new   CmdQMeta(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sQInfo;    g[v].np->deserialise(); break;
            case NODE_TYPE_ALTDST       : g[v].np = (node_ptr) new   DestList(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sDstList;  g[v].np->deserialise(); break;
            case NODE_TYPE_QBUF         : g[v].np = (node_ptr) new CmdQBuffer(g[v].name, g[v].patName, g[v].bpName, x->hash, x->cpu, x->b, flags); g[v].type = dnt::sQBuf; break;
            case NODE_TYPE_UNKNOWN      : sErr << "not yet implemented " << g[v].type << std::endl; break;
            default                     : sErr << "Node type 0x" << std::hex << type << " not supported! " << std::endl;
          }
          
        }
      }
    }

  


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // create edges

    //Two-pass for edges. First, iterate all non meta-types to establish block -> dstList parenthood
    for(auto& it : at.getTable().get<Hash>()) {
      // handled by visitor
      if (g[it.v].np == nullptr) {throw std::runtime_error( std::string("Node ") + g[it.v].name + std::string("not initialised")); return;

      } else {

        if  (!(g[it.v].np->isMeta())) g[it.v].np->accept(VisitorDownloadCrawler(g, it.v, at, sLog, sErr));
      }  
    }
    //second, iterate all meta-types
    for(auto& it : at.getTable().get<Hash>()) {
      // handled by visitor
      if (g[it.v].np == nullptr) {throw std::runtime_error( std::string("Node ") + g[it.v].name + std::string("not initialised")); return; 
      } else {
        if  (g[it.v].np->isMeta()) g[it.v].np->accept(VisitorDownloadCrawler(g, it.v, at, sLog, sErr));
      }  
    }

    
  }  

    //TODO assign to CPUs/threads

  void CarpeDM::readMgmtLLMeta() {
    vEbrds er;
    vBuf vDl;
    uint32_t modAdrBase = atDown.getMemories()[0].extBaseAdr + atDown.getMemories()[0].sharedOffs + SHCTL_META;
    er.va.push_back(modAdrBase + T_META_START_PTR);
    er.va.push_back(modAdrBase + T_META_CON_SIZE);
    er.vcs += leadingOne(2);
    
    vDl = ebReadCycle(ebd, er.va, er.vcs);
    atDown.setMgmtLLstartAdr(writeBeBytesToLeNumber<uint32_t>((uint8_t*)&vDl[T_META_START_PTR]));
    atDown.setMgmtLLsize(writeBeBytesToLeNumber<uint32_t>((uint8_t*)&vDl[T_META_CON_SIZE]));



  }

  int CarpeDM::download() {
    vBuf vDlBmpD, vDlD;
    //FIXME get mgmt linked list meta data
    

    
    vEbrds erBmp = gatherDownloadBmpVector();
    vEbrds erData;


    atDown.clear();
    atDown.clearMemories();
    gDown.clear();
    //get all BMPs so we know which nodes to download
    if(verbose) sLog << "Downloading ...";
    vDlBmpD = ebReadCycle(ebd, erBmp.va, erBmp.vcs);
    /*
    sLog << "Tried to read " << std::dec << erBmp.va.size() << " bmp addresses " << std::endl;
    sLog << "Got back " << std::dec << vDlBmpD.size() << " bmp bytes " << std::endl;
    hexDump("bmps", vDlBmpD);
    */
    atDown.setBmps( vDlBmpD );
    erData = gatherDownloadDataVector();
    vDlD   = ebReadCycle(ebd, erData.va, erData.vcs);
    /*
    sLog << "Tried to read " << erData.va.size() << " data addresses " << std::endl;
    sLog << "Got back " << vDlD.size() << " data bytes " << std::endl;
    */
    // read out current time for upload mod time (seconds, but probably better to use same format as DM FW. Convert to ns)
    modTime = getDmWrTime() * 1000000000ULL;

    if(verbose) sLog << "Done." << std::endl << "Parsing ...";
    readMgmtLLMeta(); // we have to do this before parsing
    parseDownloadMgmt(vDlD);
    parseDownloadData(vDlD);
    if(verbose) sLog << "Done." << std::endl;
    
    freshDownload = true;

    return vDlD.size();
  }


