testcase = TestCase(
  "Test0",
  [ Op("Init0", "cmd", "halt"),
    Op("Init1", "sched", "clear"),
    Op("Init2", "sched", "add", "0", "test0_sched.dot"),
    Op("Test0_0", "cmd", "-i", "0", "test0_cmd.dot"),
    Op("Test0_1", "cmd", "rawqueue BLOCK_IN0", "0", "", "test0_1_exp.txt"),
  ]
)