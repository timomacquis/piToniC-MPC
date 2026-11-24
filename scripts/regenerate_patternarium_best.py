"""1.1.2 : reconvertit les 150 kits Patternarium BEST déjà tirés (SELECTION.json), sans nouveau tirage.

Mêmes sources, même ordre, mêmes noms et fichiers .xpl ; seules les valeurs suivent les règles
d'import courantes (convert() de convert_microtonic_private.py, identique au lecteur C src/mtpreset.h).
Usage : python3 scripts/regenerate_patternarium_best.py [moteur x86_64 .so]
La sélection d'origine (1.1.1) est conservée dans Private/PatternariumBest150-1.1.1/.
"""
import ctypes,hashlib,json,math,sys,zipfile
import xml.etree.ElementTree as E
from pathlib import Path
from config import B,FIELDS
from convert_microtonic_private import Engine,group,fields,level_db,convert,parse_name,state_of,write_preset
bank=B/'Banks/PatternariumBest150';presets=bank/'Presets'
archive=B/'reference/presets/Patternarium - Best and Worst G04-G28.zip'
so=Path(sys.argv[1]) if len(sys.argv)>1 else B/'build/pitonic-x86_64.so'
sel=json.loads((bank/'SELECTION.json').read_text());report=json.loads((bank/'CONVERSION-REPORT.json').read_text())
assert hashlib.sha256(archive.read_bytes()).hexdigest()==sel['archive_sha256']
assert [k['file'] for k in report['kits']]==[r['file'] for r in sel['presets']]
lib=ctypes.CDLL(str(so));lib.mpc_engine.restype=ctypes.POINTER(Engine);api=lib.mpc_engine().contents;engine=api.create(None);assert engine
seen=set();changed=0
try:
 with zipfile.ZipFile(archive) as z:
  for row,kit in zip(sel['presets'],report['kits']):
   raw=z.read(row['source']);assert hashlib.sha256(raw).hexdigest()==row['source_sha256'],row['file']
   patches=group(raw.decode('utf-8-sig'),'DrumPatches');src=[fields(group(patches,str(i))) for i in range(1,9)]
   attenuation=max(0,max(level_db(p['Level']) for p in src));voices=[];notes=[]
   for i,p in enumerate(src):
    adjusted=[];voices.append(convert(p,attenuation,adjusted));notes.append({'voice':i+1,'source_name':parse_name(p['Name']),'adjustments':adjusted})
   api.set(engine,b'state',state_of(voices));buf=ctypes.create_string_buffer(32768)
   assert api.get(engine,b'state_valid',buf,32768)>0 and buf.value==b'1',row['file']
   assert api.get(engine,b'state',buf,32768)>0;canonical=buf.value+b'\0';digest=hashlib.sha256(canonical).hexdigest()
   assert digest not in seen,('état dupliqué',row['file']);seen.add(digest)
   for v,voice in enumerate(voices):
    for key,want in voice.items():
     t=ctypes.create_string_buffer(64);assert api.get(engine,f'v{v}_{key}'.encode(),t,64)>0;assert math.isclose(float(t.value),want,rel_tol=1e-6,abs_tol=1e-5),(row['file'],v,key)
   write_preset(presets/row['file'],row['name'],canonical);assert E.parse(presets/row['file']).findtext('preset')==row['name']
   changed+=row['state_sha256']!=digest;row['state_sha256']=digest
   kit.update(state_sha256=digest,level_attenuation_db=attenuation,voices=notes)
finally:api.destroy(engine)
assert len(seen)==150 and len(list(presets.glob('*.xpl')))==150
sel['conversion_rules']='1.1.2';report['conversion_rules']='1.1.2'
(bank/'SELECTION.json').write_text(json.dumps(sel,ensure_ascii=False,indent=2)+'\n');(bank/'CONVERSION-REPORT.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
print(f'150 kits reconvertis (règles 1.1.2), {changed} états modifiés ; noms, fichiers et ordre inchangés.')
