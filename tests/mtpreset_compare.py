#!/usr/bin/env python3
"""Compare the C .mtpreset reader (src/mtpreset.h) with converted references.

1. The 150 Patternarium BEST kits: each source .mtpreset is read from the local
   archive and converted by the C reader; every value must equal, as text
   (%.9g of the engine float), both A and B banks of the validated .xpl state.
2. Private fixtures in Private/mtpreset-tests/ (never published): compared with
   convert() from scripts/convert_microtonic_private.py, rounded the way the
   engine stores it.

Usage: python3 tests/mtpreset_compare.py build/mtpreset-test
Private material stays local: the archive and fixtures are skipped if absent.
"""
import json,struct,subprocess,sys,tempfile,zipfile
from pathlib import Path
import xml.etree.ElementTree as ET
B=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(B/'scripts'))
from convert_microtonic_private import decode,group,fields,level_db,convert
from config import FIELDS
ARCHIVE=B/'reference/presets/Patternarium - Best and Worst G04-G28.zip'
BANK=B/'Banks/PatternariumBest150'
FIXTURES=B/'Private/mtpreset-tests'

def run(tool,path):
 out=subprocess.run([tool,'--dump',str(path)],capture_output=True,text=True,check=True).stdout.split('\n')
 if not out[0].startswith('OK '):raise AssertionError(f'{path.name}: {out[0]}')
 return out[0],[line.split() for line in out[1:9]]
def engine_text(x):
 # Converter path: double -> '%.9g' -> strtof in the engine -> '%.9g' in the state.
 f=struct.unpack('<f',struct.pack('<f',float('%.9g'%x)))[0]
 return '%.9g'%f
def xpl_banks(path):
 state=ET.parse(path).getroot().find('state').text.strip()
 bank=decode(state);size=struct.unpack('>I',bank[156:160])[0];chunk=bank[160:160+size].split(b'\0')[0].decode()
 t=chunk.split();assert t[0]=='PT1',path.name;voices=[];at=2
 for v in range(8):
  at+=3;voices.append((t[at:at+24],t[at+24:at+48]));at+=48
 return voices
def compare(name,got,want_a,want_b=None):
 bad=[]
 for v in range(8):
  for f in range(24):
   for label,want in (('A',want_a[v]),('B',(want_b or want_a)[v])):
    if got[v][f]!=want[f]:bad.append(f'{name} voix {v+1} {FIELDS[f][0]} banque {label}: C {got[v][f]} / attendu {want[f]}')
 return bad

def main():
 tool=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else B/'build/mtpreset-test'
 failures=[];checked=0
 if ARCHIVE.exists():
  report=json.loads((BANK/'CONVERSION-REPORT.json').read_text())
  with zipfile.ZipFile(ARCHIVE) as archive,tempfile.TemporaryDirectory() as tmp:
   for kit in report['kits']:
    source=Path(tmp)/'kit.mtpreset';source.write_bytes(archive.read(kit['source']))
    head,got=run(tool,source);banks=xpl_banks(BANK/'Presets'/kit['file'])
    failures+=compare(kit['file'],got,[a for a,_ in banks],[b for _,b in banks])
    att=float(head.split()[4]);assert abs(att-kit['level_attenuation_db'])<1e-5,kit['file']
    checked+=1
  print(f'{"PASS" if not failures else "FAIL"} {checked} kits Patternarium BEST : valeurs C identiques aux .xpl (banques A et B)')
 else:print('SKIP archive Patternarium absente :',ARCHIVE)
 fixtures=sorted(FIXTURES.glob('*.mtpreset')) if FIXTURES.exists() else []
 for path in fixtures:
  text=path.read_text(encoding='utf-8-sig');patches=group(text,'DrumPatches')
  src=[fields(group(patches,str(i))) for i in range(1,9)];att=max(0,max(level_db(p['Level']) for p in src))
  want=[]
  for p in src:
   d=convert(p,att,[]);want.append([engine_text(d[f[0]]) for f in FIELDS])
  _,got=run(tool,path);bad=compare(path.name,got,want);failures+=bad
  print(f'{"PASS" if not bad else "FAIL"} {path.name}')
 if not fixtures:print('SKIP aucun fichier privé dans',FIXTURES)
 for line in failures[:40]:print('  ',line)
 if len(failures)>40:print(f'   ... {len(failures)-40} autres écarts')
 sys.exit(1 if failures else 0)
if __name__=='__main__':main()
