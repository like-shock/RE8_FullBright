import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def pe_fixture():
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHI', data, 0x84, 0x8664, 1, 12345)
    struct.pack_into('<H', data, 0x94, 240)
    struct.pack_into('<H', data, 0x98, 0x20b)
    struct.pack_into('<II', data, 0x98+56, 0x2000, 0x200)
    sec = 0x98+240
    data[sec:sec+8] = b'.text\0\0\0'
    struct.pack_into('<IIII', data, sec+8, 0x100, 0x1000, 0x200, 0x200)
    struct.pack_into('<I', data, sec+36, 0x60000020)
    return data


class IdentityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.bin = Path(cls.tmp.name) / 'identity'
        result = subprocess.run(['gcc', '-std=c11', '-D_POSIX_C_SOURCE=200809L', '-Wall', '-Wextra', '-Werror',
                                 '-Isrc', 'tests/build_identity_host.c', 'src/build_identity.c',
                                 'src/sha256.c', '-Wl,--wrap=fread', '-o', str(cls.bin)],
                                cwd=ROOT, capture_output=True, text=True)
        if result.returncode:
            raise AssertionError('Identity harness build failed:\n'+result.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_data(self, data, mode='identify'):
        path = Path(self.tmp.name) / 'file.bin'
        path.write_bytes(data)
        r = subprocess.run([str(self.bin), mode, str(path)], capture_output=True, text=True)
        return r, json.loads(r.stdout)

    def test_identify_hashes_full_file(self):
        data = pe_fixture() + b'overlay' * 10000
        r, out = self.run_data(data)
        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(out['sha256'], hashlib.sha256(data).hexdigest())
        self.assertEqual((out['machine'], out['timestamp'], out['image_size']), (0x8664, 12345, 8192))

    def test_truncated_headers_rejected(self):
        for n in [0, 1, 63, 127, 151, 391, 431]:
            with self.subTest(n=n):
                r, _ = self.run_data(pe_fixture()[:n])
                self.assertNotEqual(r.returncode, 0)

    def test_wrong_machine_magic_and_signatures_rejected(self):
        for offset, value in [(0, b'ZZ'), (0x80, b'NOPE'), (0x84, b'\x4c\x01'), (0x98, b'\x0b\x01')]:
            data = pe_fixture(); data[offset:offset+len(value)] = value
            self.assertNotEqual(self.run_data(data)[0].returncode, 0)

    def test_section_file_and_virtual_bounds(self):
        for offset, value in [(0x3c, 0xfffffff0), (0x98+56, 4096),
                              (0x98+240+20, 1023), (0x98+240+12, 8191), (0x98+60, 100)]:
            data=pe_fixture(); struct.pack_into('<I', data, offset, value)
            self.assertNotEqual(self.run_data(data)[0].returncode, 0)

    def test_modification_during_hash_is_rejected(self):
        r, out = self.run_data(pe_fixture() + bytes(200000), 'mutate')
        self.assertNotEqual(r.returncode, 0, out)
        self.assertEqual(out['error'], 'file_changed')

    def test_sha256_padding_and_block_boundaries(self):
        for size in [0, 1, 3, 55, 56, 63, 64, 65, 127, 128, 65537]:
            data = bytes((i * 31) % 256 for i in range(size))
            r, out = self.run_data(data, 'sha')
            self.assertEqual(r.returncode, 0, out)
            self.assertEqual(out['sha256'], hashlib.sha256(data).hexdigest())


if __name__ == '__main__': unittest.main()
