#ifndef _GROUP_TABLE_H_
#define _GROUP_TABLE_H_

#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream>
#include <set>
#include <boost/optional.hpp>
#include <boost/container/vector.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "dotstr.h"
#include "common.h"


using boost::multi_index_container;
using namespace boost::multi_index;


using namespace DotStr::Misc;

struct GroupMeta {
  std::string node;  //name of pattern
  std::string pattern;  //name of pattern
  bool patternEntry, patternExit;
  std::string beamproc; //name of entry node
  bool beamprocEntry, beamprocExit;

  
  GroupMeta(const std::string& node) : node(node), pattern(sUndefined),  patternEntry(false), patternExit(false), beamproc(sUndefined), beamprocEntry(false), beamprocExit(false) {}
  GroupMeta() : node(sUndefined), pattern(sUndefined),  patternEntry(false), patternExit(false), beamproc(sUndefined), beamprocEntry(false), beamprocExit(false) {}

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
      ar & node;
      ar & pattern;
      ar & patternEntry;
      ar & patternExit;
      ar & beamproc;
      ar & beamprocEntry;
      ar & beamprocExit;


  }
  
};

//necessary to avoid confusion with classnames elsewhere
namespace Groups {

struct Node{};
struct Pattern{};
struct Beamproc{};


}




typedef boost::multi_index_container<
  GroupMeta,
  indexed_by<
    ordered_unique <
      tag<Groups::Node>,  BOOST_MULTI_INDEX_MEMBER(GroupMeta, std::string, node)>,
    ordered_non_unique <
      tag<Groups::Pattern>,  BOOST_MULTI_INDEX_MEMBER(GroupMeta, std::string, pattern)>,
    ordered_non_unique <
      tag<Groups::Beamproc>,  BOOST_MULTI_INDEX_MEMBER(GroupMeta, std::string, beamproc)>  
  >    
 > GroupMeta_set;

typedef GroupMeta_set::iterator pmI;
typedef std::pair<pmI, pmI> pmRange;




class GroupTable {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
      ar & a;
  }
  
  GroupMeta_set a;


public:

  GroupTable(){};
  ~GroupTable(){};

  std::string store();
  void load(const std::string& s);

  bool insert(const std::string& sNode);

  template <typename Tag>
  pmRange lookup(const std::string& s)   {auto test = a.get<Tag>().equal_range(s); return pmRange({a.iterator_to(*test.first), a.iterator_to(*test.second)});}
  
  //Lookup a node, create if non existent. Single return value as node names are uniqu./d
  pmI lookupOrCreateNode(const std::string& sNode);


  bool isOk(pmI it) const {return (it != a.end()); }

  template <typename Tag>
  bool contains(const std::string& s) const {return isOk(lookup<Tag>(s));}

  template <typename Tag>
  bool remove(const std::string& s) {auto it = a.get<Tag>().erase(s); return (it != a.end() ? true : false);};

  void clear() { a.clear(); }

  void setPattern (pmI it, const std::string& sNew, bool entry, bool exit)  { a.modify(it, [sNew, entry, exit](GroupMeta& p){p.pattern  = sNew; p.patternEntry  = entry; p.patternExit  = exit;}); } 
  void setPattern (pmI it, const std::string& sNew) { setPattern(it, sNew, false, false); }
  void setPattern (const std::string& sNode, const std::string& sNew, bool entry, bool exit) { setPattern(lookupOrCreateNode(sNode), sNew, entry, exit); }
  void setPattern (const std::string& sNode, const std::string& sNew) { setPattern(sNode, sNew, false, false); }
  
  void setBeamproc (pmI it, const std::string& sNew, bool entry, bool exit)  { a.modify(it, [sNew, entry, exit](GroupMeta& p){p.beamproc = sNew; p.beamprocEntry = entry; p.beamprocExit = exit;}); } 
  void setBeamproc (pmI it, const std::string& sNew) { setBeamproc(it, sNew, false, false); }
  void setBeamproc (const std::string& sNode, const std::string& sNew, bool entry, bool exit) { setBeamproc(lookupOrCreateNode(sNode), sNew, entry, exit); } ;
  void setBeamproc (const std::string& sNode, const std::string& sNew) { setBeamproc(sNode, sNew, false, false); }
  
  template <typename Tag, std::string GroupMeta::*group>
  vStrC getGroups(const std::string& sNode) {vStrC res; pmRange x  = lookup<Tag>(sNode); if (x.first != a.end()) {for (auto it = x.first; it != x.second; ++it) res.push_back(*it.*group);} return res;}

  template <typename Tag, bool GroupMeta::*point>
  vStrC getGroupNodes(const std::string& s) {vStrC res; pmRange x  = lookup<Tag>(s); 
    if (x.first != a.end()) {
      for (auto it = x.first; it != x.second; ++it) {
        if (*it.*point) {
          std::cout << "found " << it->node << std::endl;
          res.push_back(it->node);
        }
      }   
    }
    std::cout << " res " << res.size() << std::endl; 
    return res;
  }

  template <typename Tag>
  vStrC getMembers(const std::string& s) {vStrC res; pmRange x  = lookup<Tag>(s); if (x.first != a.end()) {for (auto it = x.first; it != x.second; ++it) res.push_back(it->node);} return res;}

  vStrC getPatternEntryNodes(const std::string& sPattern)    {auto res = getGroupNodes<Groups::Pattern, &GroupMeta::patternEntry>(sPattern); std::cout << " res1 " << res.size() << std::endl; return res;};
  vStrC getPatternExitNodes(const std::string& sPattern)     {return getGroupNodes<Groups::Pattern, &GroupMeta::patternExit>(sPattern); };
  vStrC getBeamprocEntryNodes(const std::string& sBeamproc)  {return getGroupNodes<Groups::Pattern, &GroupMeta::beamprocEntry>(sBeamproc); };
  vStrC getBeamprocExitNodes(const std::string& sBeamproc)   {return getGroupNodes<Groups::Pattern, &GroupMeta::beamprocExit>(sBeamproc); };



//
  

  


  const GroupMeta_set& getTable() const { return a; }
  const size_t getSize()          const { return a.size(); }


  void debug();


};

#endif