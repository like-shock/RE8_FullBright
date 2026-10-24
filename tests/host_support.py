from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]


def compile_host(owner,name,sources,extra=()):
    temp=tempfile.TemporaryDirectory();owner.addClassCleanup(temp.cleanup)
    binary=Path(temp.name)/name
    result=subprocess.run(['gcc','-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror',
                           '-g','-Isrc',*sources,*extra,'-o',str(binary)],cwd=ROOT,capture_output=True,text=True)
    if result.returncode:raise AssertionError(result.stderr)
    return binary
