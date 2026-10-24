import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from host_support import ROOT, compile_host


class JournalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.binary=compile_host(cls,'journal',['tests/journal_host.c','src/journal.c','src/file_io.c','src/runtime.c'],['-pthread'])

    def test_rotation_overflow_and_stop(self):
        with tempfile.TemporaryDirectory() as temp:
            r=subprocess.run([str(self.binary),'normal',temp],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            files=list(Path(temp).glob('*.jsonl'))
            self.assertEqual(len(files),2)
            self.assertLessEqual(sum(p.stat().st_size for p in files),8*1024*1024)
            self.assertGreater(sum(p.stat().st_size for p in files),4*1024*1024)
            for p in files:
                for line in p.read_text().splitlines():
                    item=json.loads(line);self.assertEqual(item['status'],'OFF')
            summary=subprocess.run(['python3',str(ROOT/'tools/summarize_run.py'),*[str(p) for p in files]],capture_output=True,text=True)
            self.assertEqual(summary.returncode,0,summary.stderr)
            self.assertGreater(json.loads(summary.stdout)['events'],0)

    def test_write_failure_disables_only_logging(self):
        with tempfile.TemporaryDirectory() as temp:
            r=subprocess.run([str(self.binary),'full',temp],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)


if __name__=='__main__':unittest.main()
