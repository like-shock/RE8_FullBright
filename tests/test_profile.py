import subprocess
import unittest
from host_support import compile_host


class ProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls): cls.binary=compile_host(cls,'profile',['tests/profile_host.c','src/profile.c'])

    def test_reject_mismatch_ambiguity_sections_overlap_and_stale_output(self):
        result=subprocess.run([str(self.binary)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)


if __name__=='__main__':unittest.main()
