"""Fetch hash-pinned source inputs. Never repair a changed cache silently."""
import hashlib
import io
import json
from pathlib import Path
import tarfile
import urllib.request

ROOT=Path(__file__).resolve().parents[1]


def verify_file(path, expected):
    actual=hashlib.sha256(Path(path).read_bytes()).hexdigest()
    if actual != expected:
        raise ValueError(f'Hash mismatch: {path}; remove the changed cache explicitly before fetching again')


def fetch():
    cache=ROOT/'build/deps'
    for item in json.loads((ROOT/'deps/lock.json').read_text())['files']:
        dest=cache/item['path']
        if not dest.exists():
            with urllib.request.urlopen(item['url'],timeout=45) as response:
                data=response.read()
            if hashlib.sha256(data).hexdigest()!=item['sha256']:
                raise ValueError('Downloaded hash mismatch: '+item['path'])
            dest.parent.mkdir(parents=True,exist_ok=True)
            # Do not overwrite an existing source from another build process.
            with dest.open('xb') as file: file.write(data)
        verify_file(dest,item['sha256'])
    archive=cache/'lua-5.4.3.tar.gz'
    with tarfile.open(fileobj=io.BytesIO(archive.read_bytes()),mode='r:gz') as tar:
        members=[]
        for member in tar.getmembers():
            path=Path(member.name)
            if not (member.isfile() and len(path.parts)==3 and path.parts[:2]==('lua-5.4.3','src')
                    and path.suffix in {'.c','.h'}): continue
            members.append(member)
        expected={cache/member.name for member in members}
        source_dir=cache/'lua-5.4.3/src'
        unexpected=sorted(p for p in source_dir.rglob('*')
                          if p.suffix in {'.c','.h'} and p not in expected)
        if unexpected:
            raise ValueError(f'Unexpected cached Lua source: {unexpected[0]}; remove it explicitly before building')
        for member in members:
            path=Path(member.name)
            data=tar.extractfile(member).read()
            dest=cache/path
            if dest.exists():
                verify_file(dest,hashlib.sha256(data).hexdigest())
            else:
                dest.parent.mkdir(parents=True,exist_ok=True)
                with dest.open('xb') as file:file.write(data)
    print('Verified pinned REFramework header and Lua sources')


if __name__=='__main__':fetch()
