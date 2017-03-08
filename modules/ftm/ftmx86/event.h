#ifndef _EVENT_H_
#define _EVENT_H_


#include <stdlib.h>
#include <stdint.h>

class Event {
protected:  
  uint64_t tOffs;
  uint16_t flags;

public:
  Event() {}
  Event(uint64_t tOffs, uint16_t flags) : tOffs(tOffs), flags(flags) {}
  ~Event() {};
  virtual void show(void) = 0;
  virtual void show(uint32_t cnt, const char* sPrefix) = 0;
  //virtual uint8_t* serialise(uint8_t *buf);

};

class TimingMsg : public Event {
  uint64_t id;
  uint64_t par;
  uint32_t tef;

public:
  TimingMsg(uint64_t tOffs, uint16_t flags, uint64_t id, uint64_t par, uint32_t tef) : Event (tOffs, flags), id(id), par(par), tef(tef) {}
  ~TimingMsg() {};
  void show(void);
  void show(uint32_t cnt, const char* sPrefix);
  //uint8_t* serialise(uint8_t *buf);
};

class Command : public Event {
  uint64_t tValid;
  uint32_t act;

public:
  Command(uint64_t tOffs, uint16_t flags, uint64_t tValid, uint32_t act) : Event (tOffs, flags), tValid(tValid), act(act) {}
  ~Command() {};
  void show(void);
  void show(uint32_t cnt, const char* sPrefix);
  //uint8_t* serialise(uint8_t *buf);
};



#endif
