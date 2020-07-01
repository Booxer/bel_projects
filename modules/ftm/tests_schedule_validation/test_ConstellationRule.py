import unittest
import subprocess
import sys
import dm_testbench
import re

global data_master

"""
Class collects unit tests for the ConstellationRules in validation.cpp.
"""
class TestConstellationRule(unittest.TestCase):

  def setUp(self):
    """
    Set up for all test cases: store the arguments in class variables.
    """
    self.data_master = data_master

  def checkScheduleFail(self, dot_file, regex_pattern):
    """
    Common method for a ConstellationRule. Negative case.
    Add the pattern and check the negative result as an expected RuntimeError with correct message.
    """
    with self.assertRaises(RuntimeError) as context:
      dm_testbench.startpattern(self.data_master, dot_file)
    self.assertTrue(re.search(regex_pattern, str(context.exception)), f'Exception: {str(context.exception)}, pattern: {regex_pattern}')

  def test_TMsg_DefDest_Fail(self):
    self.checkScheduleFail('PatternConstellationRules/TMsg_DefDest_Fail.dot', r'Validation of Neighbourhood: Node (.)+ of type (.)+ cannot be childless')

  def test_TMsg_DefDest_Fail_AltDest(self):
    self.checkScheduleFail('PatternConstellationRules/TMsg_DefDest_Fail_AltDest.dot', r'Validation of Neighbourhood: Node (.)+ of type (.)+ must not have edge of type (.)+')

  def checkScheduleOk(self, dot_file):
    """
    Common method for a ConstellationRule. Positive case.
    Add the pattern and check the positive result.
    """
    dm_testbench.startpattern(self.data_master, dot_file)

  def test_TMsg_DefDest(self):
    self.checkScheduleOk('PatternConstellationRules/TMsg_DefDest.dot')

if __name__ == '__main__':
  if len(sys.argv) > 1:
#    print(f"Arguments: {sys.argv}")
    data_master = sys.argv.pop()
#    print(f"Arguments: {sys.argv}, {len(sys.argv)}")
    unittest.main(verbosity=2)
  else:
    print(f'Required argument missing: {sys.argv}')
