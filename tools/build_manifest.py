"""Record source and binary provenance; not a gameplay validation certificate."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def main():
    inputs=[]
    for directory in ['src','scripts','deps','tools']:
        inputs.extend(p for p in (ROOT/directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    manifest={
        'schema':1, 'stage':'foundation', 'supported_profiles':0,
        'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        'source_dirty':bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True).strip()),
        'compiler':(ROOT/'build/compiler.txt').read_text(),
        'dll_sha256':hashlib.sha256((ROOT/'build/RE8_FullBright.dll').read_bytes()).hexdigest(),
        'source_files':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(inputs)},
    }
    (ROOT/'build/manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__=='__main__':main()
