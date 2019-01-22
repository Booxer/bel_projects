#DM Test Manager v 0.1
import difflib
import sys
import os.path
import subprocess

prefix ="/usr/local/bin/dm-"

class Op:
  def __init__(self, desc, typ, content, execTime=0, optFile=None, expResFile=None):
    self.desc       = desc
    self.typ        = typ
    self.content    = content
    self.execTime   = execTime
    self.optFile    = optFile
    self.expResFile = expResFile
    self.expRes     = None
    self.path       = None
    self.result     = None    
  
  def setPath(self, path):
    self.path = path


  def getOptFileName(self):
    if self.optFile is not None:
      return self.path + "/" + self.optFile
    return ""    

  def getCliStr(self, dev):
    execStr = prefix + self.typ + " " + dev + " " + self.content + " " + self.getOptFileName();
    return execStr

  def getCliCmd(self):
    execStr = prefix + self.typ
    return execStr
    
  def getCliArgs(self, dev):
    execList = [dev]
    execList += self.content.split()
    execList.append(self.getOptFileName())
    return execList
    #execStr = " " + dev + " " + self.content
    #if self.optFile is not None:
    #  execStr += " " + self.path + self.optFile
    #return execStr      

  def getExpRes(self):
    if expRes is None and expResFile is not None:
      with open(self.path + self.expResFile) as f:
        self.expRes = f.readlines()
    return expRes

  def runEb(self, dev):
    res = ""
    optFile = ""
    if self.optFile is not None:
      optFile = self.path + self.optFile
    try:   
      tmp = subprocess.check_output([self.getCliCmd()] + self.getCliArgs(dev), stderr=subprocess.STDOUT)
      if self.expResFile is not None:
        self.result = str(tmp)
    except subprocess.CalledProcessError as err:
      print("Rcode %s Stdout %s" % (err.returncode, err.output)) 

       
    #sleep(int(self.execTime))

  def runRda(self, instance):
    pass  

  def showRes(self):
    print(self.result)   


class TestCase:
  def __init__(self, name, opList):
    self.name = name
    self.path = None
    self.opList = opList
    self.result = None

  def showOpsList(self):
    for op in self.opList:
      optFile = "-"
      if op.optFile is not None:
        optFile = self.path + "/" +  op.optFile  
      expResFile = "-"
      if op.expResFile is not None:
        expResFile = self.path + "/" +  op.expResFile  
      print("# %s, '%s', %ums, %s %s" % (op.desc, op.getCliStr(self.dev), int(op.execTime), optFile, expResFile))

  def setPath(self, filename):
    if os.path.isfile(filename):
      mypath, myfile = os.path.split(filename)        
      if not mypath:
        mypath += './'
      self.path = mypath
      for op in self.opList:
        op.setPath(self.path)
    else:
      print("Manifest contains invalid path in <%s>" % filename)

  def setDev(self, dev):
    self.dev = dev

  def runOp(self, index, mode="eb"):
    if mode == "eb":
      self.opList[index].runEb(self.dev)
    else:
      self.opList[index].runRda(self.rda)  
    self.opList[index].showRes()

  def run(self, mode="eb"):
    for op in self.opList:
      if mode == "eb":
        op.runEb(self.dev)
      else:
        op.runRda(self.rda)
      op.showRes()

  def eval(self, index=-1):
    if index == -1:
      for op in self.opList:
        if op.expResFile is not None:
          tmp = op.diff()
        else:
          print("-")
    else:
      if self.opList[index].expResFile is not None:
          tmp = self.opList[index].diff()
        else:
          print("-")       
  

        
 # def runOp    


class Manager:
  def __init__(self, manifestFile, dev):
    exec(open(manifestFile).read(), locals())
    self.tests        = locals()["testfiles"]
    self.dev          = dev
    self.actTestCase  = None #TestCase("hallo", [Op("Init0", "cmd", "halt"),])
    self.actResult    = []



  def showTestList(self):
    print(self.tests)

  def loadTest(self, index):
    if index < len(self.tests):
      exec(open(self.tests[index]).read(), globals(), locals())
      self.actTestCase = locals()["testcase"]
      self.actTestCase.setPath(self.tests[index])
      self.actTestCase.setDev(self.dev)
    else:
      print("Index not in list of tests")   

  #def runTest(self):
  #  if self.actTestCase is not None:


def main(argv):
  print("Loading Test")
  m = Manager("TestManifest.py", "tcp/tsl008.acc")
  m.showTestList()
  m.loadTest(0)
  m.actTestCase.showOpsList()
  m.actTestCase.run()


if __name__ == "__main__":
  main(sys.argv[1:])


