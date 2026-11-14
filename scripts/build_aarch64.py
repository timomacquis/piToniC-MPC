"""Build/package Gen2 from the released Gen1 skin and identical engine inputs. No device access."""
from pathlib import Path
import os,sys,json,hashlib,subprocess,zipfile,shutil,re,xml.etree.ElementTree as ET
from PIL import Image
from config import B,FOLDER,VERSION,DESCRIPTION
os.chdir(B)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
receipt=json.loads((B/'Banks/PatternariumBest150/Compiled/BUILD.json').read_text())
for name,digest in receipt['inputs'].items():assert sha(B/name)==digest,'Changed published engine input: '+name
assert receipt['programs']==150
out=B/'build/gen2';out.mkdir(parents=True,exist_ok=True)
root=Path(os.environ.get('PITONIC_AARCH64_SYSROOT','/home/greg/forcework/gen2-toolchain/root'))
env=dict(os.environ,PATH=str(root/'usr/bin')+':'+os.environ['PATH'],LD_LIBRARY_PATH=str(root/'usr/lib/x86_64-linux-gnu')+':'+os.environ.get('LD_LIBRARY_PATH',''))
flags=['-std=c11','-D_POSIX_C_SOURCE=200809L','-O2','-fPIC','-shared','-DPITONIC_FACTORY_PROGRAMS=1','-IBanks/PatternariumBest150/Generated','-Ibuild','-Isrc','-Ithird_party/mpc-wrapper/wrapper']
so=out/'pitonic.so'
subprocess.run([str(root/'usr/bin/aarch64-linux-gnu-gcc-12'),'--sysroot='+str(root),'-B'+str(root/'usr/aarch64-linux-gnu/lib')+'/',*flags,'src/engine.c','third_party/mpc-wrapper/wrapper/vst2_wrap.c','-static-libgcc','-lpthread','-lm','-ldl','-Wl,--no-undefined','-Wl,-z,relro,-z,now,-z,noexecstack','-o',str(so)],env=env,check=True)
subprocess.run([str(root/'usr/bin/aarch64-linux-gnu-strip'),str(so)],env=env,check=True)
report=subprocess.check_output([str(root/'usr/bin/aarch64-linux-gnu-readelf'),'-h','-d','--version-info',str(so)],env=env,text=True)
(out/'ELF.txt').write_text(report)
raw=so.read_bytes();assert raw[:5]==b'\x7fELF\x02' and int.from_bytes(raw[18:20],'little')==183
versions=set(re.findall(r'GLIBC_([0-9.]+)',report));assert all(tuple(map(int,v.split('.')))<=(2,34) for v in versions),versions
source=B/f'dist/{VERSION}/piToniC-{VERSION}-mpc-armv7.zip';original=sha(source)
stage=out/FOLDER
if stage.exists():shutil.rmtree(stage)
stage.mkdir()
with zipfile.ZipFile(source) as z:
 prefix=next(n[:-len('Plugin Skins/TUI.json')] for n in z.namelist() if n.endswith('/Plugin Skins/TUI.json'))
 for n in z.namelist():
  if n.startswith(prefix) and not n.endswith('/'):
   rel=n[len(prefix):]
   if rel=='pitonic.so':continue
   dest=stage/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(z.read(n))
assets=stage/'Plugin Skins';tui=assets/'TUI.json';j=json.loads(tui.read_text());grown={}
def walk(x):
 if isinstance(x,dict):
  if x.get('type')=='Image' and isinstance(x.get('data'),dict) and x['data'].get('image'):
   name=x['data']['image']
   if name not in grown:
    im=Image.open(assets/name);w,h=im.size;new=name
    if w*h<6400:
     pw=max(w,80);ph=max(h,(6400+pw-1)//pw);canvas=Image.new(im.mode,(pw,ph),(0,0,0,0) if im.mode=='RGBA' else im.getpixel((0,0)));canvas.paste(im,(0,0));new='g2_'+name;canvas.save(assets/new)
    grown[name]=new
   x['data']['image']=grown[name]
  for v in x.values():walk(v)
 elif isinstance(x,list):
  for v in x:walk(v)
walk(j);tui.write_text(json.dumps(j,indent=2)+'\n')
notes='Gen2 ARM64 build of πToniC '+VERSION+'. Compiled and package-checked on PC; NOT TESTED ON GEN2 HARDWARE. 150 factory presets. Small Image assets padded for Gen2 rendering. Gen1 package unchanged. Please report device model, OS and results for sound, presets, Q-Links, samples and effects.\n'
(stage/'GEN2-STATUS.md').write_text(notes)
(stage/'PROVENANCE.json').write_text(json.dumps(dict(version=VERSION,architecture='aarch64',hardware_verified=False,gen1_package_sha256=original,engine_sha256=sha(so),source_hashes=receipt['inputs'],small_images_padded=sum(k!=v for k,v in grown.items())),indent=2)+'\n')
entry=out/'entry.xml';entry.write_text(ET.tostring(ET.Element('PLUGIN',name='πToniC',descriptiveName='πToniC',format='VST',category='Synth',manufacturer='Timo',version=VERSION,file='%payload-path%/'+FOLDER+'/pitonic.so',uid='50695443',isInstrument='1',numInputs='0',numOutputs='2'),encoding='unicode'))
dest=B/'dist'/VERSION
subprocess.run([sys.executable,str(B/'third_party/open-mpc-release/tools/release.py'),'--so',str(so),'--skin',str(stage),'--entry',str(entry),'--version',VERSION,'--id','pitonic','--repo','timomacquis/piToniC-MPC','--license','MIT AND LicenseRef-Patternarium','--about',DESCRIPTION,'-o',str(dest)],check=True)
z=dest/f'ToniC-{VERSION}-mpc-aarch64.zip';target=dest/f'piToniC-{VERSION}-mpc-aarch64.zip';z.replace(target)
subprocess.run([sys.executable,str(B/'third_party/open-mpc-release/tools/catalog_check.py'),str(target),'--catalog','--require-portable','--expect-id','pitonic','--expect-repo','timomacquis/piToniC-MPC'],check=True)
with zipfile.ZipFile(target) as z:
 assert z.testzip() is None
 assert sum(n.endswith('.xpl') for n in z.namelist())==150
 sums=next(n for n in z.namelist() if n.endswith('/SHA256SUMS'));top=sums.rsplit('/',1)[0]
 for line in z.read(sums).decode().splitlines():
  digest,name=line.split('  ',1);assert hashlib.sha256(z.read(top+'/'+name)).hexdigest()==digest,name
assert sha(source)==original
Path(str(target)+'.sha256').write_text(sha(target)+'  '+target.name+'\n')
(out/'RESULT.json').write_text(json.dumps(dict(package=str(target),sha256=sha(target),hardware_verified=False,gen1_unchanged=True,glibc_versions=sorted(versions),presets=150,images_padded=sum(k!=v for k,v in grown.items())),indent=2)+'\n')
print(notes, target,sha(target))