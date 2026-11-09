"""Package the 150 selected Patternarium BEST kits as native programs and XPLs."""
from pathlib import Path
import json,hashlib,shutil,subprocess,sys,zipfile
from config import B,FOLDER,VERSION
import os
os.chdir(B);bank=B/'Banks/PatternariumBest150';selection=json.loads((bank/'SELECTION.json').read_text());receipt=json.loads((bank/'Compiled/BUILD.json').read_text());assert selection['count']==receipt['programs']==150 and not receipt['private_only']
for p,d in receipt['inputs'].items():assert hashlib.sha256((B/p).read_bytes()).hexdigest()==d,'Rebuild required: '+p
compiled=bank/'Compiled/pitonic.so';assert hashlib.sha256(compiled.read_bytes()).hexdigest()==receipt['outputs']['pitonic.so']
skin=bank/'Runtime'/FOLDER
if skin.exists():shutil.rmtree(skin)
shutil.copytree(B/'build/skin'/FOLDER,skin);shutil.copytree(bank/'Presets',skin/'Presets');(skin/'docs').mkdir();(skin/'licenses').mkdir()
for src,dst in [('LICENSE','piToniC-MIT.txt'),('third_party/mpc-wrapper/LICENSE','wrapper-MIT.txt'),('third_party/weirddrums/LICENSE','WeirdDrums-MIT.txt'),('reference/fonts/OFL.txt','Titillium-OFL.txt')]:shutil.copy2(B/src,skin/'licenses'/dst)
shutil.copy2(B/'THIRD-PARTY.md',skin/'licenses/THIRD-PARTY.md')
for name in ['SELECTION.json','CONVERSION-REPORT.json','NAMES.md']:shutil.copy2(bank/name,skin/name)
(skin/'PATTERNARIUM-CREDITS.md').write_text('''# Preset source credits\n\n150 kits selected from the Best of Generation 4–28 folders of the Patternarium archive supplied by Greg. Original generated sounds: Sonic Charge / Patternarium, for Microtonic. Converted parameter recipes, not the Microtonic DSP. Factory Microtonic presets are not part of this bank. All original filenames, generations and hashes are retained in SELECTION.json.\n\nSource-use references: https://soniccharge.com/forum/topic/1336 (Patternarium description and public-domain statement, 2019) and https://soniccharge.com/forum/topic/2562-selling-sample-packs-made-with-sonic-charge-plugins (2023 statement about informing Sonic Charge and attribution for Patternarium use in sample packs). No publisher correspondence or separate permission is claimed here. Keep these source credits with the bank.\n''')
manual=(B/'docs/MODE-EMPLOI.md').read_text().split('### Charger les presets de la banque privée')[0]
manual+='''\n## Banque embarquée — 150 kits Patternarium BEST\n\nLa flèche PRESET en haut à droite liste les 150 kits embarqués. Un choix charge les huit voix et leurs états A/B. Le premier kit est chargé à l’insertion du plugin. Les modifications et le programme sélectionné sont enregistrés avec le projet.\n\nLa banque provient uniquement des dossiers Best of Generation G04–G28 : six kits par génération, 1 200 sons, noms courts. Les réglages sont adaptés au moteur de πToniC ; aucune identité sonore avec Microtonic n’est revendiquée.\n\nLes mêmes kits sont disponibles au format XPL dans le navigateur User → Force Documents → Plugin Presets → Instruments → Timo - VST - πToniC. Les anciens fichiers personnels ne sont pas écrasés.\n'''
(skin/'docs/MODE-EMPLOI.md').write_text(manual)
shutil.copy2(B/'docs/EFFECTS-IMPLEMENTATION.md',skin/'docs/EFFECTS-IMPLEMENTATION.md')
(skin/'docs/images').mkdir()
for image in (B/'docs/images').glob('*.png'):shutil.copy2(image,skin/'docs/images'/image.name)
subprocess.run([sys.executable,'third_party/open-mpc-release/tools/release.py','--so',str(compiled),'--skin',str(skin),'--entry','build/pluginlist-entry.xml','--version',VERSION,'--id','pitonic','--license','MIT AND LicenseRef-Patternarium','--about','Eight-voice drum synthesis with 150 converted Patternarium BEST kits.','-o',str(bank/'Build')],check=True)
old=bank/'Build'/('ToniC-'+VERSION+'-mpc-armv7.zip');target=bank/'Build'/('piToniC-'+VERSION+'-Patternarium-BEST150-mpc-armv7.zip');old.replace(target)
with zipfile.ZipFile(target) as z:entries=[(i,z.read(i.filename)) for i in z.infolist()]
root=entries[0][0].filename.split('/')[0];files={i.filename:d for i,d in entries};key=root+'/install.sh';script=files[key].decode();extra='''# Also expose shipped XPLs in the Force user Load browser, preserving personal files.
USER_PRESETS="/sdcard/Force Documents/Plugin Presets/Instruments/$SKIN"
mkdir -p "$USER_PRESETS"
for pf in "$NEW"/Presets/*.xpl; do
 [ -f "$pf" ] || continue
 dest="$USER_PRESETS/$(basename "$pf")"
 if [ ! -e "$dest" ]; then cp -p "$pf" "$dest"; fi
done
''';assert '\nsync\n' in script;files[key]=script.replace('\nsync\n','\n'+extra+'\nsync\n',1).encode();subprocess.run(['sh','-n'],input=files[key],check=True)
files[root+'/SHA256SUMS']=''.join(hashlib.sha256(d).hexdigest()+'  '+n[len(root)+1:]+'\n' for n,d in sorted(files.items()) if n!=root+'/SHA256SUMS').encode()
with zipfile.ZipFile(target,'w') as z:
 for i,d in entries:z.writestr(i,files[i.filename])
with zipfile.ZipFile(target) as z:
 assert z.testzip() is None;assert sum('/Presets/' in n and n.endswith('.xpl') for n in z.namelist())==150
 for row in z.read(root+'/SHA256SUMS').decode().splitlines():
  d,n=row.split(None,1);assert hashlib.sha256(z.read(root+'/'+n.lstrip('*'))).hexdigest()==d,n
 assert hashlib.sha256(z.read(root+'/portable/'+FOLDER+'/pitonic.so')).hexdigest()==receipt['outputs']['pitonic.so']
Path(str(target)+'.sha256').write_text(hashlib.sha256(target.read_bytes()).hexdigest()+'  '+target.name+'\n');print('Verified candidate package:',target)
