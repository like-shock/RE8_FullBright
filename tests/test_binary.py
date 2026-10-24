import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT=Path(__file__).resolve().parents[1]


class BinaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll=ROOT/'build/RE8_FullBright.dll'
        if not cls.dll.exists():
            if os.environ.get('FB_REQUIRE_BINARY')=='1': raise AssertionError('required Windows DLL missing')
            raise unittest.SkipTest('Windows build not present; required in CI')

    def test_exports_and_no_non_system_runtime_imports(self):
        tool=shutil.which('x86_64-w64-mingw32-objdump') or shutil.which('objdump')
        self.assertIsNotNone(tool)
        r=subprocess.run([tool,'-p',str(self.dll)],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertIn('pei-x86-64',r.stdout)
        for name in ['reframework_plugin_initialize','reframework_plugin_required_version']:
            self.assertRegex(r.stdout, r'\b'+name+r'\b')
        imports={x.lower() for x in re.findall(r'DLL Name:\s*(\S+)',r.stdout)}
        self.assertTrue(imports)
        self.assertLessEqual(imports,{'kernel32.dll','msvcrt.dll','user32.dll'})

    def test_manifest_describes_actual_binary_and_sources(self):
        manifest=json.loads((ROOT/'build/manifest.json').read_text())
        self.assertEqual(manifest['dll_sha256'],hashlib.sha256(self.dll.read_bytes()).hexdigest())
        self.assertEqual(manifest['supported_profiles'],0)
        self.assertEqual(manifest['stage'],'foundation')
        self.assertRegex(manifest['source_commit'],r'^[0-9a-f]{40}$')
        for path,digest in manifest['source_files'].items():
            self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(),digest,path)


if __name__=='__main__':unittest.main()
