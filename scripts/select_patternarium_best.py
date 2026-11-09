"""Draw 150 distinct sound kits exclusively from exact Best folders from the user-provided Patternarium archive.
No Microtonic factory patches or binaries are included in this selection.
"""
from pathlib import Path
import random,secrets,json,hashlib,zipfile,ctypes,math,re,collections
import xml.etree.ElementTree as E
from config import B,FIELDS
from convert_microtonic_private import Engine,group,fields,number,level_db,convert,parse_name,state_of,write_preset
archive=B/'reference/presets/Patternarium - Best and Worst G04-G28.zip';out=B/'Banks/PatternariumBest150';presets=out/'Presets';presets.mkdir(parents=True,exist_ok=True)
archive_hash=hashlib.file_digest(archive.open('rb'),'sha256').hexdigest();seedfile=out/'DRAW.json'
if seedfile.exists():
 draw=json.loads(seedfile.read_text());assert draw['archive_sha256']==archive_hash;seed=int(draw['seed_hex'],16)
else:
 seed=secrets.randbits(128);draw={'archive_sha256':archive_hash,'seed_hex':hex(seed),'method':'Six randomly drawn kits per generation G04–G28, exact Best of Generation folders only; duplicate converted sound states rejected.'};seedfile.write_text(json.dumps(draw,indent=2)+'\n')
rng=random.Random(seed);lib=ctypes.CDLL(str(B/'build/pitonic-x86_64.so'));lib.mpc_engine.restype=ctypes.POINTER(Engine);api=lib.mpc_engine().contents;engine=api.create(None);assert engine
adjectives=['Ash','Ion','Neo','Ice','Red','Dry','Low','Raw','Air','Tin','Jet','Orb','Bit','Arc','Fog'];nouns=['Pulse','Dust','Circuit','Echo','Spark','Bloom','Grain','Drift','Metal','Orbit'];rows=[];reports=[];seen=set();rejected=[]
try:
 with zipfile.ZipFile(archive) as z:
  candidates=collections.defaultdict(list)
  for info in z.infolist():
   matches=[re.fullmatch(r'Best of Generation (\d+)',part,re.I) for part in Path(info.filename).parts[:-1]]
   matched=[m for m in matches if m]
   if matched and info.filename.lower().endswith('.mtpreset'):
    assert len(matched)==1;generation=int(matched[0].group(1));candidates[f'G{generation:02}'].append(info.filename)
  assert sorted(candidates)==[f'G{i:02}' for i in range(4,29)];assert sum(map(len,candidates.values()))==512
  for generation in sorted(candidates):
   bag=sorted(candidates[generation]);rng.shuffle(bag);taken=0
   for source in bag:
    if taken==6:break
    raw=z.read(source);text=raw.decode('utf-8-sig');patches=group(text,'DrumPatches');src=[fields(group(patches,str(i))) for i in range(1,9)];attenuation=max(0,max(level_db(p['Level']) for p in src));voices=[];notes=[]
    for i,p in enumerate(src):
     adjusted=[];voices.append(convert(p,attenuation,adjusted));notes.append({'voice':i+1,'source_name':parse_name(p['Name']),'adjustments':adjusted})
    chunk=state_of(voices);api.set(engine,b'state',chunk);buf=ctypes.create_string_buffer(32768);assert api.get(engine,b'state_valid',buf,32768)>0 and buf.value==b'1',source
    assert api.get(engine,b'state',buf,32768)>0;canonical=buf.value+b'\0';digest=hashlib.sha256(canonical).hexdigest()
    if digest in seen:rejected.append({'source':source,'reason':'duplicate converted sound state'});continue
    seen.add(digest);index=len(rows);label=adjectives[index//10]+' '+nouns[index%10];filename=label.replace(' ','')+'.xpl';assert len(label)<15 and len(filename)<15
    write_preset(presets/filename,label,canonical)
    assert E.parse(presets/filename).findtext('preset')==label
    for v,voice in enumerate(voices):
     for key,want in voice.items():
      temp=ctypes.create_string_buffer(64);assert api.get(engine,f'v{v}_{key}'.encode(),temp,64)>0;assert math.isclose(float(temp.value),want,rel_tol=1e-6,abs_tol=1e-5)
    row={'index':index,'name':label,'file':filename,'generation':generation,'original_name':Path(source).stem,'source':source,'source_sha256':hashlib.sha256(raw).hexdigest(),'state_sha256':digest};rows.append(row);reports.append(dict(row,level_attenuation_db=attenuation,voices=notes));taken+=1
   assert taken==6,generation
finally:api.destroy(engine)
assert len(rows)==150 and len(seen)==150 and len(list(presets.glob('*.xpl')))==150
manifest={**draw,'count':150,'kit_count':150,'sound_count':1200,'extra_init_included':False,'generation_counts':dict(collections.Counter(r['generation'] for r in rows)),'max_display_name_length':max(len(r['name']) for r in rows),'max_filename_length':max(len(r['file']) for r in rows),'presets':rows,'rejected':rejected}
(out/'SELECTION.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n');(out/'CONVERSION-REPORT.json').write_text(json.dumps({'source_family':'Patternarium BEST G04–G28','preset_count':150,'kit_count':150,'converted_sound_count':1200,'archive_sha256':archive_hash,'kits':reports,'licensing_sources':['https://soniccharge.com/forum/topic/1336','https://soniccharge.com/forum/topic/2562-selling-sample-packs-made-with-sonic-charge-plugins'],'publication_performed':False},ensure_ascii=False,indent=2)+'\n')
(out/'NAMES.md').write_text('# Patternarium BEST selection — 150 kits\n\nSix kits drawn exclusively from Best folders in each generation, 150 kits / 1200 voices. No extra Init entry. All display names and filenames including .xpl are shorter than 15 characters. Source names, archive hashes and the random seed are preserved in SELECTION.json and DRAW.json.\n\n'+ '\n'.join(f'{i+1:03}. {r["name"]} — {r["generation"]} — {r["original_name"]}' for i,r in enumerate(rows))+'\n')
print('Selected and converted 150 distinct BEST kits / 1200 voices; six per generation; compiled-engine state/range checks passed.');print('Seed:',hex(seed));print('Output:',out)
