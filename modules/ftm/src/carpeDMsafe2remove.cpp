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
#include "carpeDM.h"
#include "propwrite.h"

#include "node.h"
#include "block.h"
#include "dotstr.h"

namespace dnt = DotStr::Node::TypeVal;
namespace det = DotStr::Edge::TypeVal;

namespace isSafeToRemove {
  const std::string exIntro = "isSafeToRemove: ";
}


bool CarpeDM::isSafeToRemove(Graph& gRem, std::string& report, bool optimise) {
  std::set<std::string> patterns;
  //Find all patterns 2B removed
  for (auto& patternIt : getGraphPatterns(gRem)) { patterns.insert(patternIt); }
    
  return isSafeToRemove(patterns, report, optimise);
}

bool CarpeDM::isSafeToRemove(const std::string& pattern, std::string& report, bool optimise) {
  std::set<std::string> p = {pattern};
  return isSafeToRemove(p, report, optimise);
}  


bool CarpeDM::isSafeToRemove(std::set<std::string> patterns, std::string& report, bool optimise) {
  bool isSafe = true;
  Graph& g        = gDown;
  AllocTable& at  = atDown;
  CovenantTable ctAux; //Global CovenantTable is called ct
  Graph gTmp, gEq;
  vertex_set_t blacklist, entries, cursors, covenants; //hashes of all covenants
  vertex_set_map_t covenantsPerVertex;
  std::string optmisedAnalysisReport, covenantReport;

  //if(verbose) {sLog << "Pattern <" << pattern << "> (Entrypoint <" << sTmp << "> safe removal analysis" << std::endl;}

  for (auto& patternIt : patterns) {
    // BEGIN Preparations: Entry points, Blacklist and working copy of the Graph
    //Init our blacklist of critical nodes. All vertices in the pattern to be removed need to be on it
    for (auto& nodeIt : getPatternMembers(patternIt)) {
      if (hm.lookup(nodeIt)) {
        auto x = at.lookupHash(hm.lookup(nodeIt).get());
        if (!(at.isOk(x))) {throw std::runtime_error(isSafeToRemove::exIntro +  "Could not find pattern <" + patternIt + "> member node <" + nodeIt + ">"); return false;}
        blacklist.insert(x->v);
        covenantsPerVertex[x->v].insert(null_vertex);
      }
    }
    //Find and list all entry nodes of patterns 2B removed
    std::string sTmp = getPatternEntryNode(patternIt);
    if (hm.lookup(sTmp)) {
      auto x = at.lookupHash(hm.lookup(sTmp).get());
      if (!(at.isOk(x))) {throw std::runtime_error(isSafeToRemove::exIntro + "Could not find pattern <" + patternIt + "> entry node <" + sTmp + ">"); return false;}
      entries.insert(x->v);
    }
  

  }

  //make a working copy of the download graph
  vertex_map_t vertexMapTmp;
  boost::associative_property_map<vertex_map_t> vertexMapWrapperTmp(vertexMapTmp);
  copy_graph(g, gTmp, boost::orig_to_copy(vertexMapWrapperTmp));
  for (auto& it : vertexMapTmp) { //check vertex indices
    if (it.first != it.second) {throw std::runtime_error(isSafeToRemove::exIntro +  "CpyGraph Map1 Idx Translation failed! This is beyond bad, contact Dev !");}
  }
  
  // End Preparations

  //BEGIN Basic Static equivalent model //
  //add static equivalent edges of all pending flow commands to working copy  
  if (addDynamicDestinations(gTmp, at)) { if(verbose) {sLog << "Added dynamic equivalents." << std::endl;} }

  if(verbose) sLog << "Generating filtered graph view " << std::endl;


  //Generate a filtered view, stripping all edges except default Destinations, resident flow destinations and dynamic flow destinations
  typedef boost::property_map< Graph, std::string myEdge::* >::type EpMap;
  boost::filtered_graph <Graph, static_eq<EpMap>, boost::keep_all > fg(gTmp, make_static_eq(boost::get(&myEdge::type, gTmp)), boost::keep_all());
  //copy filtered view to normal graph to work with
  vertex_map_t vertexMapEq;
  boost::associative_property_map<vertex_map_t> vertexMapWrapperEq(vertexMapEq);
  copy_graph(fg, gEq, boost::orig_to_copy(vertexMapWrapperEq));
  for (auto& it : vertexMapEq) { //check vertex indices
    if (it.first != it.second) { throw std::runtime_error(isSafeToRemove::exIntro + "CpyGraph Map2 Idx Translation failed! This is beyond bad, contact Dev !");}
  }

  if(verbose) sLog << "Reading Cursors " << std::endl;
  //try to get consistent image of active cursors
  updateModTime();
  cursors = getAllCursors(true); // Set to false for debugging system behaviour with static cursors

  //Here comes the problem: resident commands are only of consquence if they AND their target Block are active
  //Iteratively find out which cmds are executable and add equivalent edges for them. Do this until no more new edges have to be added
  if (addResidentDestinations(gEq, gTmp, cursors)) { if(verbose) {sLog << "Added resident equivalents." << std::endl;} }

  // END Basic Static Equivalent Model //

  // BEGIN Optimised Static Equivalent Model
  // Under certain conditions, (offending) default destinations can be replaced
  if (optimise) {
    if(verbose) {sLog << "Starting Optimiser (Update stale defDst)" << std::endl;}
    if (updateStaleDefaultDestinations(gEq, at, ctAux, optmisedAnalysisReport)) { if(verbose) {sLog << "Updated stale Default Destinations to reduce wait time." << std::endl;} }
  }

  // END Optimised Static Equivalent Model

  //covenant: promise not to clear/reorder a given block's queues

  // Crawl and map active areas
  //crawl all reverse trees we can reach from the given entries and add their nodes to the blacklist
  for (auto& vEntry : entries) {
    if(verbose) { sLog << "Starting Crawler from " << gEq[vEntry].name << std::endl; }
    vertex_set_t tmpTree;
    getReverseNodeTree(vEntry, tmpTree, gEq, covenantsPerVertex);
    blacklist.insert(tmpTree.begin(), tmpTree.end());
  }

  if(verbose) sLog << "Judging safety " << std::endl;
  //calculate intersection of cursors and blacklist. If the intersection set is empty, all nodes in pattern can be safely removed
  vertex_set_t si;
  set_intersection(blacklist.begin(),blacklist.end(),cursors.begin(),cursors.end(), std::inserter(si,si.begin()));

  for (auto& it : blacklist)  { 
    if (verbose) { 
      sLog << gEq[it].name << "-- > {";

      for (auto& itPv : covenantsPerVertex[it]) { sLog << ((itPv != null_vertex) ? gEq[itPv].name : "NULL") << ", "; }
      sLog << std::endl;
    }

  }

  //create set of all covenants which must be honoured so the prediction will hold. Because of the propagation along reverse trees, doing it for intersection members is sufficient
  for (auto& it : si)  { 
    covenants.insert(covenantsPerVertex[it].begin(), covenantsPerVertex[it].end()); 
    if (verbose) { 
      sLog << gEq[it].name << "-- > {";

      for (auto& itPv : covenantsPerVertex[it]) { sLog << ((itPv != null_vertex) ? gEq[itPv].name : "NULL") << ", "; }
      sLog << std::endl;
    }

  }


  if(isSafetyCritical(covenants)) isSafe = false; // if a safety critical node (cov set contains NO_COVENANT) is on the intersection with cursor set, it's unsafe
 
  if(verbose) sLog << "Creating report " << std::endl;
  //Create Debug Output File
  BOOST_FOREACH( vertex_t v, vertices(gEq) ) { gEq[v].np->clrFlags(NFLG_PAINT_LM32_SMSK); }
  for (auto& it : cursors)    { gEq[it].np->setFlags(NFLG_DEBUG1_SMSK); }
  for (auto& it : entries)    { gEq[it].np->setFlags(NFLG_DEBUG0_SMSK); }
  for (auto& it : blacklist)  {
    if(!optimise || (optimise && isSafetyCritical(covenantsPerVertex[it]))) gEq[it].np->setFlags(NFLG_PAINT_HOST_SMSK);
  }
  report += createDot(gEq, true);
  
  if (optimise) {
    report += optmisedAnalysisReport;

    //if(isSafe) { //Showing a list of covenants only makes sense if they'd do us any good
      //if (covenants.size() > 0) {
        report += "//Covenants to honour:\n";
        //}
      for (auto& it : covenants)  {
        if (it == null_vertex) {report += "//None\n"; continue;}
        std::string covName = gEq[it].name;
        //find covname in ctAux and copy found entry to ct watchlist
        auto x = ctAux.lookup(covName);
        if (!ctAux.isOk(x)) { throw std::runtime_error(isSafeToRemove::exIntro + ": Lookup of <" + covName + "> in covenantAuxTable failed\n");}
        if (!ct.insert(x))  { throw std::runtime_error(isSafeToRemove::exIntro + ": Insertion of <" + covName + "> in covenantTable failed\n");} ;

        auto y = ct.lookup(covName);
        if (!ct.isOk(y)) { throw std::runtime_error(isSafeToRemove::exIntro + ": Lookup of <" + covName + "> in covenantTable failed\n");}
        
        //and report
        report += "//" + covName + " p " + std::to_string(y->prio) + " s " + std::to_string(y->slot) + " chk 0x" + std::to_string(y->chkSum) + "\n";
      }
      report += covenantReport;
    //}
  }

  sLog << report << std::endl;

  for (auto& it : blacklist)  {
    sLog << gEq[it].name << " covP " << isCovenantPending(gEq[it].name) << std::endl;
  }

  return isSafe;
}

bool CarpeDM::isOptimisableEdge(edge_t e, Graph& g) {
  return (g[e].type == det::sBadDefDst);
}

bool CarpeDM::isCovenantPending(const std::string& covName) {
  bool ret = false;
  cmI x = ct.lookup(covName);
  if (!ct.isOk(x)) { sLog << "DBG unknonwn"; return false;} //throw std::runtime_error(isSafeToRemove::exIntro + ": Lookup of <" + covName + "> in covenantTable failed\n");
  return isCovenantPending(x);
}

bool CarpeDM::isCovenantPending(cmI cov) {
  QueueReport qr;
  getQReport(cov->name, qr);
  QueueElement& qe = qr.aQ[cov->prio].aQe[cov->slot];
    
  if (cov->chkSum == ct.genChecksum(qe))  return true;
  else                                    return false;
}
/*
unsigned CarpeDM::updateCovenants() {

  unsigned cnt = 0;
  vStrC toDelete;
  for (auto it : ct.getTable()) {
    if (!isCovenantPending(it)) toDelete.push_back(it->name);
    cnt++;
  }

  for (auto it : toDelete) { ct.removed(it); }

  return cnt;
}
*/

bool CarpeDM::isSafetyCritical(vertex_set_t& c) {
  if (c.find(null_vertex) != c.end()) return true;
  else return false;
}


//recursively inserts all vertex idxs of the tree reachable (via in edges) from start vertex into the referenced set
void CarpeDM::getReverseNodeTree(vertex_t v, vertex_set_t& sV, Graph& g, vertex_set_map_t& covenantsPerVertex, vertex_t covenant) {

  Graph::in_edge_iterator in_begin, in_end, in_cur;
  //Do the crawl       
  boost::tie(in_begin, in_end) = in_edges(v,g);
  for (in_cur = in_begin; in_cur != in_end; ++in_cur) {
    if (verbose) { sLog << g[target(*in_cur, g)].name << "<-- " << g[*in_cur].type << " --" << g[source(*in_cur, g)].name  << std::endl; }
    vertex_set_t& cpvs = covenantsPerVertex[source(*in_cur, g)];   

    if (isOptimisableEdge(*in_cur, g)) {
    
      if (verbose) { sLog << " Optimisable:  " << g[source(*in_cur, g)].name << "->" << g[target(*in_cur, g)].name << std::endl; }
      //if (covenant == null_vertex) { 
        covenant = source(*in_cur, g); 
        if (verbose) sLog << "Setting covenant " << g[source(*in_cur, g)].name << std::endl;
      //} //acquire the first covenant we come accross and propagate it
     
    }  
    sV.insert(source(*in_cur, g));
    if (cpvs.find(covenant) != cpvs.end()) { 
      std::string covName = (covenant == null_vertex) ? "NULL" : g[covenant].name;
      sLog << g[source(*in_cur, g)].name <<  "'s covenant with " << covName << " already in tree, breaking" << std::endl; 
      break;
    }
    cpvs.insert(covenant);

    //if (sV.find(source(*in_cur, g)) != sV.end()) { sLog << g[source(*in_cur, g)].name <<  " already in tree, breaking" << std::endl; break;}
    
    sLog << "Adding to BlkList " << g[source(*in_cur, g)].name << std::endl;

    getReverseNodeTree(source(*in_cur, g), sV, g, covenantsPerVertex, covenant);
    
  }
}



bool CarpeDM::addResidentDestinations(Graph& gEq, Graph& gOrig, vertex_set_t cursors) {
  vertex_set_t resCmds; // prepare the set of flow commands to speed things up
  vertex_set_map_t dummy; // this doesn't need to look out for covenant sets, ignore
  BOOST_FOREACH( vertex_t vChkResCmd, vertices(gEq) ) {if (gEq[vChkResCmd].type == dnt::sCmdFlow) resCmds.insert(vChkResCmd);}
  bool addEdge = (resCmds.size() > 0);
  bool didWork = false;

  while (addEdge) {
    addEdge = false;
    for(auto& vRc : resCmds) {
      vertex_set_t tmpTree, si;
      vertex_t vBlock = -1, vDst = -1; 
      Graph::out_edge_iterator out_begin, out_end, out_cur;
      bool found = false;

      //find out if there is a path from any of the cursors to this command
      getReverseNodeTree(vRc, tmpTree, gEq, dummy);
      set_intersection(tmpTree.begin(),tmpTree.end(),cursors.begin(),cursors.end(), std::inserter(si,si.begin()));
      if ( si.size() > 0 ) {
        //found a path. now check if there already is an equivalent edge between this command's target block and its destination
        //get block and dst
        
        //We now intentionally use the unfiltered graph again (to have target and dst edges). works cause vertex indices are equal.
        boost::tie(out_begin, out_end) = out_edges(vRc, gOrig);
        for (out_cur = out_begin; out_cur != out_end; ++out_cur) {
          if(gOrig[*out_cur].type == det::sCmdTarget)  {vBlock  = target(*out_cur, gOrig);}
          if(gOrig[*out_cur].type == det::sCmdFlowDst) {vDst    = target(*out_cur, gOrig);}
        }
        if (((signed)vBlock  == -1) || ((signed)vDst == -1)) {throw std::runtime_error(isSafeToRemove::exIntro + "Could not find block and dst for resident equivalents");}
        
         //check for equivalent resident edges
        boost::tie(out_begin, out_end) = out_edges(vBlock, gEq);
        for (out_cur = out_begin; out_cur != out_end; ++out_cur) { if(gEq[*out_cur].type == det::sResFlowDst)  found = true;}
        if (!found) {
          if (verbose) { sLog << "Adding ResFlowAuxEdge: " << gEq[vBlock].name << " -> " << gEq[vDst].name << std::endl; }
          boost::add_edge(vBlock, vDst, myEdge(det::sResFlowDst), gEq);
          addEdge = true;
          didWork = true;
        }
      }
    }  
  }
  return didWork;
}

bool CarpeDM::updateStaleDefaultDestinations(Graph& g, AllocTable& at, CovenantTable& covTab, std::string& qAnalysis) {
  bool didWork = false;
  edge_t edgeToDelete;

  BOOST_FOREACH( vertex_t vChkBlock, vertices(g) ) {
    //first, find blocks
    if(g[vChkBlock].np->isBlock()) {
      //second, inspect their queues and see if default dest is made stale by a dominant flow
      vertex_set_t sVflowDst = getDominantFlowDst(vChkBlock, g, at, covTab, qAnalysis);
      if(verbose) sLog << std::endl;
      for (auto& it : sVflowDst) {
        
        //if (sVflowDst.size() > 1) {throw std::runtime_error(isSafeToRemove::exIntro + "updateStaleDefDst: found more than one dominant flow, must be 0..1");}
        if((signed)it != -1) { boost::add_edge(vChkBlock, it, myEdge(det::sDomFlowDst), g); if (verbose)  sLog << "updateStaleDefDst: Adding edge to " << g[it].name << std::endl; }
        else { if (verbose)  sLog << "updateStaleDefDst: New default would be idle, skipping edge creation" << std::endl; }
        //find old default edge and mark for deletion
        Graph::out_edge_iterator out_begin, out_end, out_cur;
        boost::tie(out_begin, out_end) = out_edges(vChkBlock, g);
        for (out_cur = out_begin; out_cur != out_end; ++out_cur) { 
          if(g[*out_cur].type == det::sDefDst) {
            if (verbose) sLog << "updateStaleDefDst: Found old default dst <" << g[target(*out_cur, g)].name << "> of block <" << g[vChkBlock].name << ">, changing type to non traversible" << std::endl;
            didWork = true;
            g[*out_cur].type = det::sBadDefDst;
          } 
        }
      }
    }
  }
  
  return didWork;
}

vertex_set_t CarpeDM::getDominantFlowDst(vertex_t vQ, Graph& g, AllocTable& at, CovenantTable& covTab, std::string& qAnalysis) {
  vertex_set_t ret;

  

  qAnalysis += "//" + g[vQ].name;

  QueueReport qr;
  qr = getQReport(g, at, g[vQ].name, qr);
        
  for (int8_t prio = PRIO_IL; prio >= PRIO_LO; prio--) {
    qAnalysis += "#P" + std::to_string((int)prio);
    if (!qr.hasQ[prio]) {qAnalysis += "->xX->xX->xX->xX"; continue;} // if the priority doesn't exist, Ignore

    for (uint8_t i, idx = qr.aQ[prio].rdIdx; idx < qr.aQ[prio].rdIdx + 4; idx++) {
      i = idx & Q_IDX_MAX_MSK;
      QueueElement& qe = qr.aQ[prio].aQe[i];

      // if flow at read idx is not pending, this queue is empty.
      if (!qe.pending) {qAnalysis +="->eE"; continue;} 

      // we're going through in order. If element has a valid time in the future (> modTime), stop right here. it and all following are possibly yet unprocessed
      if(qe.validTime > modTime) {qAnalysis += "->v" + std::to_string((int)qe.type) + "\n"; 
        qAnalysis += "//vTime " + std::to_string((int)qe.validTime) + " mTime " + std::to_string((int)modTime);
        return ret;
      } 

      if (qe.type != ACT_TYPE_FLOW) {
        qAnalysis += "->t" + std::to_string((int)qe.type) + "\n";
        //if the command is not a flow, we can stop here: It means the default will be used at least once, thus there is no dominant flow
        return ret;
      }  
      
      //found a pending flow to idle, insert bogus vertex index to show that.
      if (qe.flowDst == DotStr::Node::Special::sIdle) {
        ret.insert(-1); 
        qAnalysis += "->i" + std::to_string((int)qe.type) + "\n";
        if(verbose) sLog << "updateStaleDefDst: Found dominant flow dst idle" << std::endl;
        return ret;
      } 
      // we ruled out that the flow leads to idle. If it's not permanent, it can't be dominant. Ignore
      if (!qe.flowPerma) {qAnalysis +=  "->p" + std::to_string((int)qe.type); continue;} 
      //found a dominant flow, insert its destination
      auto x = at.lookupHash(hm.lookup(qe.flowDst).get());
      if (!(at.isOk(x))) {throw std::runtime_error(isSafeToRemove::exIntro + "updateStaleDefDst: Could not find dst in download allocation table");}
      if(verbose) sLog << "updateStaleDefDst: Found dominant flow dst " << g[x->v].name << std::endl;
      ret.insert(x->v);
      covTab.insert(g[vQ].name, (uint8_t)prio, i, qe); //save which element in which queue of which block is eligible to save our arse
      qAnalysis +=  "->D" + std::to_string((int)qe.type); 
    }
  }
  qAnalysis += "\n"; 
  return ret;
}      



bool CarpeDM::addDynamicDestinations(Graph& g, AllocTable& at) {
  bool didWork = false;

  BOOST_FOREACH( vertex_t vChkBlock, vertices(g) ) {
    //first, find blocks
    if(g[vChkBlock].np->isBlock()) {
      //second, inspect their queues and add equivalent edges for pending flows
      vertex_set_t sVflowDst = getDynamicDestinations(vChkBlock, g, at);
      for (auto& it : sVflowDst) {
        if(verbose) {sLog << "Adding DynFlowAuxEdge: " << g[vChkBlock].name << " -> " << g[it].name << std::endl;}
        boost::add_edge(vChkBlock, it, myEdge(det::sDynFlowDst), g);
        didWork = true;
      }
    }
  }
  return didWork;
}


vertex_set_t CarpeDM::getDynamicDestinations(vertex_t vQ, Graph& g, AllocTable& at) {


  vertex_set_t ret;

  if(verbose) sLog << "Searching for pending flows " << g[vQ].name << std::endl;

  QueueReport qr;
  qr = getQReport(g, at, g[vQ].name, qr);
        
  for (int8_t prio = PRIO_IL; prio >= PRIO_LO; prio--) {
    if (!qr.hasQ[prio]) {continue;}

    //find buffers of all non empty slots
    for (uint8_t i, idx = qr.aQ[prio].rdIdx; idx < qr.aQ[prio].rdIdx + 4; idx++) {
      i = idx & Q_IDX_MAX_MSK;
      QueueElement& qe = qr.aQ[prio].aQe[i];

      if (!qe.pending) {continue;}
      if (qe.type == ACT_TYPE_FLOW) {
        if (qe.flowDst == DotStr::Node::Special::sIdle) {continue;}
        auto x = at.lookupHash(hm.lookup(qe.flowDst).get());
        if (!(at.isOk(x))) {throw std::runtime_error(isSafeToRemove::exIntro + "Could not find dst in download allocation table");}
        if(verbose) sLog << "Found flow dst " << g[x->v].name << std::endl;
        ret.insert(x->v); //found a pending flow, insert its destination
      }
    }  
  }

  return ret;
}      