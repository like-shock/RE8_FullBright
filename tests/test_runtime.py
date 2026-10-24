import subprocess
import unittest
from host_support import compile_host


class RuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls): cls.binary=compile_host(cls,'runtime',['tests/runtime_host.c','src/runtime.c'],['-pthread'])

    def test_requests_observations_and_generations(self):
        result=subprocess.run([str(self.binary)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)


if __name__=='__main__':unittest.main()
