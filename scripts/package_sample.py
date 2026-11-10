"""Gen1 sample maintenance package; no install, no publish, no sample redistribution."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys,zipfile,xml.etree.ElementTree as ET
from config import B,FOLDER,VERSION,DESCRIPTION
bank=B/'Banks/PatternariumBest150';build=json.loads((bank/'Compiled/BUILD.json').read_text());assert build['programs']==150
for name,digest in build['inputs'].items():assert hashlib.sha256((B/name).read_bytes()).hexdigest()==digest,'Stale build: '+name
so=bank/'Compiled/pitonic.so';assert hashlib.sha256(so.read_bytes()).hexdigest()==build['outputs']['pitonic.so']
receipt=json.loads((B/'build/build-receipt.json').read_text())
for name,digest in receipt['inputs'].items():assert hashlib.sha256((B/name).read_bytes()).hexdigest()==digest,'Stale build: '+name
for name,digest in receipt['skin'].items():assert hashlib.sha256((B/'build/skin'/name).read_bytes()).hexdigest()==digest,'Stale skin: '+name
stage=B/'build/sample-package'/FOLDER
if stage.exists():shutil.rmtree(stage)
shutil.copytree(B/'build/skin'/FOLDER,stage)
shutil.copytree(bank/'Presets',stage/'Presets')
# Ship user guidance and images, excluding research inventories and device logs.
(stage/'docs').mkdir()
for name in ['USER-MANUAL.md','MODE-EMPLOI.md','SAMPLE-LCD-1.0.2.md','RELEASE-NOTES-1.0.2.md','PRODUCT-DESCRIPTION.json']:
 shutil.copy2(B/'docs'/name,stage/'docs'/name)
shutil.copytree(B/'docs/images',stage/'docs/images')
(stage/'licenses').mkdir()
for src,dst in [('LICENSE','piToniC-MIT.txt'),('THIRD-PARTY.md','THIRD-PARTY.md'),('third_party/mpc-wrapper/LICENSE','wrapper-MIT.txt'),('third_party/weirddrums/LICENSE','WeirdDrums-MIT.txt'),('reference/fonts/OFL.txt','Titillium-OFL.txt')]:shutil.copy2(B/src,stage/'licenses'/dst)
for name in ['SELECTION.json','CONVERSION-REPORT.json','NAMES.md']:shutil.copy2(bank/name,stage/name)
credits=B/'Releases/1.0.0/Public/PATTERNARIUM-CREDITS.md'
if not credits.exists():credits=bank/'Runtime'/FOLDER/'PATTERNARIUM-CREDITS.md'
assert credits.exists();shutil.copy2(credits,stage/'PATTERNARIUM-CREDITS.md')
(stage/'PROVENANCE.json').write_text(json.dumps({'version':VERSION,'status':'release based on installed Gen1 candidate.2; exact-release install/uninstall not newly qualified','architecture':'armv7','programs':150,'engine_sha256':build['outputs']['pitonic.so'],'hardware_access':False,'samples_included':False,'source_hashes':build['inputs']},indent=2)+'\n')
entry=B/'build/sample-entry.xml';entry.write_text(ET.tostring(ET.Element('PLUGIN',name='πToniC',descriptiveName='πToniC',format='VST',category='Synth',manufacturer='Timo',version=VERSION.split('-',1)[0],file='%payload-path%/'+FOLDER+'/pitonic.so',uid='50695443',isInstrument='1',numInputs='0',numOutputs='2'),encoding='unicode'))
runtime_version=VERSION.split('-',1)[0]
out=B/'dist'/VERSION;out.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(B/'third_party/open-mpc-release/tools/release.py'),'--so',str(so),'--skin',str(stage),'--entry',str(entry),'--version',runtime_version,'--id','pitonic','--repo','timomacquis/piToniC-MPC','--license','MIT AND LicenseRef-Patternarium','--about',DESCRIPTION,'-o',str(out)],check=True)
z=out/('ToniC-'+runtime_version+'-mpc-armv7.zip');target=out/('piToniC-'+VERSION+'-mpc-armv7.zip');z.replace(target)
subprocess.run([sys.executable,str(B/'third_party/open-mpc-release/tools/catalog_check.py'),str(target),'--catalog','--require-portable','--expect-id','pitonic','--expect-repo','timomacquis/piToniC-MPC'],check=True)
with zipfile.ZipFile(target) as archive:
 assert archive.testzip() is None
 sums=next(n for n in archive.namelist() if n.endswith('/SHA256SUMS'));top=sums.rsplit('/',1)[0]
 for line in archive.read(sums).decode().splitlines():
  digest,name=line.split('  ',1);assert hashlib.sha256(archive.read(top+'/'+name)).hexdigest()==digest,name
 assert sum(n.endswith('.xpl') for n in archive.namelist())==150
 raw=archive.read(next(n for n in archive.namelist() if n.endswith('/pitonic.so')));assert raw[:5]==b'\x7fELF\x01' and int.from_bytes(raw[18:20],'little')==40
 assert not any(n.lower().endswith('.wav') for n in archive.namelist())
digest=hashlib.sha256(target.read_bytes()).hexdigest();Path(str(target)+'.sha256').write_text(digest+'  '+target.name+'\n')
(B/('Validation/sample-'+VERSION+'/PACKAGE.json')).write_text(json.dumps({'version':VERSION,'file':str(target),'sha256':digest,'bytes':target.stat().st_size,'integrity_and_payload_hashes':True,'architecture':'Gen1 ARM32','factory_programs':150,'hardware_verified':False,'installed':False,'published':False},indent=2)+'\n')
print('Verified local package:',target,digest)
