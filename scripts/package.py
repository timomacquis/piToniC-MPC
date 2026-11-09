"""Package verified local outputs with pinned Open MPC release tools; never install."""
import json,hashlib,shutil,subprocess,sys,zipfile
from config import B,FOLDER,VERSION
from pathlib import Path
import os
os.chdir(B)
receipt=json.loads((B/'build/build-receipt.json').read_text())
for path,digest in receipt['inputs'].items():assert hashlib.sha256((B/path).read_bytes()).hexdigest()==digest,'Rebuild needed: '+path
for path,digest in receipt.get('skin',{}).items():assert hashlib.sha256((B/'build/skin'/path).read_bytes()).hexdigest()==digest,'Rebuild skin receipt: '+path
assert hashlib.sha256((B/'build/pitonic.so').read_bytes()).hexdigest()==receipt['outputs']['pitonic.so']
sys.path.insert(0,str(B/'third_party/mpc-wrapper/tools'));import gen_vst
(B/'build/pluginlist-entry.xml').write_text(gen_vst.entry(gen_vst.load(str(B/'vst.json')))+'\n')
lic=B/'build/package-licenses';lic.mkdir(exist_ok=True)
for src,dest in [('LICENSE','LICENSE.txt'),('THIRD-PARTY.md','THIRD-PARTY.md'),('third_party/weirddrums/LICENSE','WeirdDrums-MIT.txt'),('third_party/weird-dreams/README.md','Weird-Dreams-README.md'),('third_party/mpc-wrapper/LICENSE','wrapper-MIT.txt'),('third_party/open-mpc-release/LICENSE','release-tools-MIT.txt'),('reference/fonts/OFL.txt','Titillium-OFL.txt')]:shutil.copy2(B/src,lic/dest)
docs=B/'build/package-docs';docs.mkdir(exist_ok=True)
for name in ['MODE-EMPLOI.md','IMPLEMENTATION.md']:shutil.copy2(B/'docs'/name,docs/name)
subprocess.run([sys.executable,'third_party/open-mpc-release/tools/release.py','--so','build/pitonic.so','--skin',str(B/'build/skin'/FOLDER),'--entry','build/pluginlist-entry.xml','--version',VERSION,'--id','pitonic','--license','MIT','--about','Eight-voice drum synthesis instrument; first device-test candidate.','--extra',str(lic)+':licenses','--extra',str(docs)+':docs','-o','dist'],check=True)
files=list((B/'dist').glob('ToniC-'+VERSION+'*.zip'));assert len(files)==1
src=files[0];target=B/'dist'/('piToniC-'+VERSION+'-mpc-armv7.zip');src.replace(target)
with zipfile.ZipFile(target) as z:
 assert not z.testzip();names=z.namelist();root=names[0].split('/')[0];sums=z.read(root+'/SHA256SUMS').decode()
 for row in sums.splitlines():
  digest,path=row.split(None,1);path=path.lstrip('*');assert hashlib.sha256(z.read(root+'/'+path)).hexdigest()==digest,path
(B/'dist'/(target.name+'.sha256')).write_text(hashlib.sha256(target.read_bytes()).hexdigest()+'  '+target.name+'\n')
print('ZIP integrity and all packaged checksums verified:',target.name)
