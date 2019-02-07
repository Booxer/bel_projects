#Test for no flow, 0 flows, 1 flow, 10 flows

testcase = TestCase(
  "Flush standard, flush with override",
  [ Op("Init0_0", "cmd", "reset all"),
    Op("Init0_1", "sched", "add", "0", "test_sched.dot"),
    Op("Test0_2_CheckA", "cmd", "-i", "0.0", "test0_cmd.dot"),
    Op("Test0_0_RunCPU0", "cmd", "startpattern LOOP"),
    Op("Test0_1_CheckA", "sched", "rawvisited", "0.1", None, "test0_1_exp.txt"),
    Op("Test0_2_CheckA", "cmd", "rawstatus", "0.1", None, "test0_2_exp.txt"),
    Op("Init1_0", "cmd", "reset all"),
    Op("Init1_1", "sched", "add", "0", "test_sched.dot"),
    Op("Test1_2_CheckA", "cmd", "-i", "0.0", "test1_cmd.dot"),
    Op("Test1_0_RunCPU0", "cmd", "startpattern LOOP"),
    Op("Test1_1_CheckA", "sched", "rawvisited", "0.1", None, "test1_1_exp.txt"),
    Op("Test1_2_CheckA", "cmd", "rawstatus", "0.1", None, "test1_2_exp.txt"),
    Op("Init2_0", "cmd", "reset all"),
    Op("Init2_1", "sched", "add", "0", "test_sched.dot"),
    Op("Test2_0_RunCPU0", "cmd", "startpattern LOOP"),
    Op("Test2_2_CheckA", "cmd", "rawstatus", "0.1", None, "test2_2_exp.txt"),

  ]
)