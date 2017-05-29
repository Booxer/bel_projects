#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/container/vector.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/endian/conversion.hpp>


#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 



class Node;

typedef boost::shared_ptr<Node> node_ptr;
typedef boost::container::vector<node_ptr> npBuf;
typedef boost::container::vector<uint8_t> vBuf;
typedef boost::container::vector<uint32_t> vAdr;


class FnvHash
{
    static const unsigned int FNV_PRIME     = 16777619u;
    static const unsigned int OFFSET_BASIS  = 2166136261u;
public:    
    static unsigned int fnvHash(const char* str)
    {
        const size_t length = strlen(str) + 1;
        unsigned int hash = OFFSET_BASIS;
        for (size_t i = 0; i < length; ++i)
        {
            hash ^= *str++;
            hash *= FNV_PRIME;
        }
        return hash;
    }
 
};

typedef struct {
  std::string name;
  uint32_t hash;
  node_ptr np;

  //FIXME
  //now follows a list of all possible properties graphviz_read can assign, to copy to concrete Node objects later
  //dirty business. this will have to go in the future
  // Option 1 (easy and clean, still needs ALL possible properties to be present in myVertex):
  //    pass Node constructor a reference to hosting myVertex Struct, Node can then use all relevent myVertex properties.
  //    Will make Node constructors VERY simple
  // Option 2 (hard, but very clean): 
  //    overload graphviz_read subfunctions so the parser evaluates typefield first, then have a 
  //    Node class factory and directly reference derived class members in property map.
  std::string type;

  uint32_t flags;

  //Meta

  //Block 
  uint64_t tPeriod;
  uint8_t rdIdxIl, rdIdxHi, rdIdxLo;
  uint8_t wrIdxIl, wrIdxHi, wrIdxLo;

  //Event
  uint64_t tOffs;

  //Timing Message
  uint64_t id;
  uint64_t par;
  uint32_t tef;
  uint32_t res;

  //Command

  uint64_t tValid;


  // Flush

  bool qIl, qHi, qLo;

  uint8_t frmIl, toIl;
  uint8_t frmHi, toHi;
  uint8_t frmLo, toLo; 

  //Flow, Noop
  uint16_t qty;

  //Wait
  uint64_t tWait;

} myVertex;




typedef struct {
  std::string type;
} myEdge;


typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::bidirectionalS, myVertex, myEdge > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

 

template<typename T>
inline void writeLeNumberToBeBytes(uint8_t* pB, T val) {
  T x = boost::endian::endian_reverse(val);
  std::copy(static_cast<const uint8_t*>(static_cast<const void*>(&x)),
            static_cast<const uint8_t*>(static_cast<const void*>(&x)) + sizeof x,
            pB);
}

template<typename T>
inline T writeBeBytesToLeNumber(uint8_t* pB) {
  return boost::endian::endian_reverse(*((T*)pB));
}


#endif
