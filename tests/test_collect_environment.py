import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CollectorTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.game = self.root / 'steamapps/common/RE8 game'
        self.game.mkdir(parents=True)
        (self.game / 're8.exe').write_bytes(b'fake PE fixture\x00\xff')
        (self.game / 'dinput8.dll').write_bytes(b'framework fixture')
        self.output = self.root / 'report'

    def run_collector(self):
        return subprocess.run(['bash', str(ROOT / 'tools/collect_environment.sh'),
                               str(self.game), str(self.output)], capture_output=True, text=True)

    def test_space_path_hashes_and_no_game_writes(self):
        before = {p.name: p.read_bytes() for p in self.game.iterdir()}
        result = self.run_collector()
        self.assertEqual(result.returncode, 0, result.stderr)
        report = (self.output / 'environment.txt').read_text()
        self.assertIn('exe_sha256=' + hashlib.sha256(before['re8.exe']).hexdigest(), report)
        self.assertIn('framework_sha256=' + hashlib.sha256(before['dinput8.dll']).hexdigest(), report)
        self.assertIn('proton_version=unconfirmed', report)
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.game.iterdir()})

    def test_missing_game_has_no_output(self):
        (self.game / 're8.exe').unlink()
        self.assertEqual(self.run_collector().returncode, 2)
        self.assertFalse(self.output.exists())

    def test_existing_output_is_preserved(self):
        self.output.mkdir()
        (self.output / 'environment.txt').write_text('keep me')
        self.assertNotEqual(self.run_collector().returncode, 0)
        self.assertEqual((self.output / 'environment.txt').read_text(), 'keep me')

    def test_missing_framework_is_not_claimed_installed(self):
        (self.game / 'dinput8.dll').unlink()
        result = self.run_collector()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('framework_sha256=missing', (self.output / 'environment.txt').read_text())

    def test_output_cannot_be_inside_game(self):
        self.output = self.game / 'report'
        self.assertNotEqual(self.run_collector().returncode, 0)
        self.assertFalse(self.output.exists())

    def test_matching_steam_manifest(self):
        manifest = self.root / 'steamapps/appmanifest_1196590.acf'
        manifest.write_text('"AppState"\n{\n "appid" "1196590"\n "installdir" "RE8 game"\n "buildid" "12345678"\n}\n')
        result = self.run_collector()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('steam_build_id=12345678', (self.output / 'environment.txt').read_text())

    def test_wrong_manifest_is_not_trusted(self):
        (self.root / 'steamapps/appmanifest_1196590.acf').write_text('"appid" "1196590"\n"installdir" "Other"\n"buildid" "42"\n')
        result = self.run_collector()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('steam_build_id=unconfirmed', (self.output / 'environment.txt').read_text())


if __name__ == '__main__':
    unittest.main()
