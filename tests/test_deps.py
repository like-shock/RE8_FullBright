import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]


class DependencyTests(unittest.TestCase):
    def test_modified_cache_is_rejected_not_silently_replaced(self):
        path=ROOT/'tools/fetch_deps.py'
        self.assertTrue(path.exists(), 'dependency verifier missing')
        spec=importlib.util.spec_from_file_location('fetch_deps',path)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as temp:
            dest=Path(temp)/'file'
            dest.write_bytes(b'original')
            expected=hashlib.sha256(b'original').hexdigest()
            module.verify_file(dest,expected)
            dest.write_bytes(b'changed')
            with self.assertRaises(ValueError): module.verify_file(dest,expected)
            self.assertEqual(dest.read_bytes(), b'changed')


if __name__=='__main__':unittest.main()
