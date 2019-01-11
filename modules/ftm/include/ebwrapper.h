


Class EbWrapper {

private:

  std::string ebdevname;
  Socket ebs;
  Device ebd;





  std::vector<uint32_t *> simRam;
  std::map<uint8_t, uint32_t> simRamAdrMap;
  std::vector<struct sdb_device> cpuDevs;
  std::vector<struct sdb_device> cluTimeDevs;
  std::vector<struct sdb_device> diagDevs;



  bool simConnect();
  bool simDisconnect();
  void simAdrTranslation (uint32_t a, uint8_t& cpu, uint32_t& arIdx);
  void simRamWrite (uint32_t a, eb_data_t d);
  void simRamRead (uint32_t a, eb_data_t* d);
  int  simWriteCycle(vAdr va, vBuf& vb);
  vBuf simReadCycle(vAdr va);

  bool ebConnect(const std::string& en, bool test=false); //Open connection to a DM via Etherbone
  bool ebDisconnect(); //Close connection
  int   ebWriteCycle(vAdr va, vBuf& vb, vBl vcs);
  int   ebWriteCycle(vAdr va, vBuf& vb);
  vBuf  ebReadCycle(vAdr va, vBl vcs);
  vBuf  ebReadCycle(vAdr va);
  int   ebWrite32b(uint32_t adr, uint32_t data);
  uint32_t ebRead32b(uint32_t adr);
  uint64_t ebRead64b(uint32_t startAdr);
  int ebWrite64b(uint32_t startAdr, uint64_t data);


public:
  //TODO any better way to do this ?
  bool connect(const std::string& en, bool simulation=false, bool test=false);
  bool disconnect(); //Close connection
  int writeCycle(const ebWrs& ew);
  int writeCycle(vAdr va, vBuf& vb, vBl vcs);
  int writeCycle(vAdr va, vBuf& vb);
  vBuf readCycle(const ebRds& er);
  vBuf readCycle(vAdr va, vBl vcs);
  vBuf readCycle(vAdr va );
  uint32_t read32b(uint32_t adr);
  uint64_t read64b(uint32_t startAdr);
  int write32b(uint32_t adr, uint32_t data);
  int write64b(uint32_t startAdr, uint64_t data);



};