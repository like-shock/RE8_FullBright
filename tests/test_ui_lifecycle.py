import subprocess
import unittest
from host_support import ROOT,compile_host


class UiLifecycleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        lua=ROOT/'build/deps/lua-5.4.3/src'
        sources=[str(p) for p in lua.glob('*.c') if p.name not in ('lua.c','luac.c')]
        cls.binary=compile_host(cls,'ui',['tests/ui_host.c','src/options_ui.c','src/options.c',
                                         'src/file_io.c','src/runtime.c',*sources],['-I'+str(lua),'-pthread','-lm'])

    def test_real_lua_attachment_stale_callbacks_and_native_request(self):
        r=subprocess.run([str(self.binary)],cwd=ROOT,capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)


if __name__=='__main__':unittest.main()
