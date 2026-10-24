import subprocess
import tempfile
from pathlib import Path
import unittest
from host_support import compile_host


class OptionsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.binary=compile_host(cls,'options',['tests/options_host.c','src/options.c','src/file_io.c'])

    def test_configuration_validation_atomic_save_and_key_latch(self):
        with tempfile.TemporaryDirectory() as tmp:
            result=subprocess.run([str(self.binary),tmp],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((Path(tmp)/'config.ini').read_text(),'schema_version=1\ntoggle_vk=120\ndetailed_log=1\n')


if __name__=='__main__':unittest.main()
