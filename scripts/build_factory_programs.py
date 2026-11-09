"""Build the 150 native factory programs for PC and MPC Gen1."""
import json,hashlib,os,subprocess,sys,struct
from pathlib import Path
import xml.etree.ElementTree as E
from config import B
from preset_codec import decode
os.chdir(B);private=(B/os.environ.get('PITONIC_BANK_ROOT','Banks/PatternariumBest150')).resolve();assert private.is_relative_to((B/'Banks').resolve());private_only=False;generated=private/'Generated';out=private/'Compiled';generated.mkdir(exist_ok=True);out.mkdir(exist_ok=True)
items=[];names=set()
selection=private/'SELECTION.json'
paths=[private/'Presets'/r['file'] for r in json.loads(selection.read_text())['presets']] if selection.exists() else sorted((private/'Presets').glob('*.xpl'))
for path in paths:
 root=E.parse(path).getroot();raw=decode(root.findtext('state'));assert raw[:4]==b'CcnK' and raw[8:12]==b'FBCh';chunk=raw[160:];assert len(chunk)==struct.unpack('>I',raw[156:160])[0] and chunk[-1:]==b'\0';state=chunk[:-1].decode('ascii');assert state.startswith('PT1 ')
 full=root.findtext('preset');name=full[:23];index=2
 while name in names:
  suffix=f' [{index}]';name=full[:23-len(suffix)]+suffix;index+=1
 names.add(name);items.append({'name':name,'full_name':full,'file':path.name,'state':state})
assert len(items)==json.loads((private/'CONVERSION-REPORT.json').read_text())['preset_count']
header=['/* Generated Patternarium factory bank; preserve source credits and provenance. */',f'#define PITONIC_FACTORY_COUNT {len(items)}','typedef struct {const char *name,*state;} PiTonicFactory;','static const PiTonicFactory PITONIC_FACTORY[PITONIC_FACTORY_COUNT]={']
for item in items:header.append('{'+json.dumps(item['name'])+','+json.dumps(item['state'])+'},')
header.append('};');(generated/'pitonic_factory_programs.h').write_text('\n'.join(header)+'\n')
(generated/'program-list.json').write_text(json.dumps([{k:v for k,v in i.items() if k!='state'} for i in items],indent=2)+'\n')
flags=['-std=c11','-D_POSIX_C_SOURCE=200809L','-O2','-fPIC','-shared','-DPITONIC_FACTORY_PROGRAMS=1','-I'+str(generated),'-Ibuild','-Isrc','-Ithird_party/mpc-wrapper/wrapper'];sources=['src/engine.c','third_party/mpc-wrapper/wrapper/vst2_wrap.c'];libs=['-lpthread','-lm','-ldl']
subprocess.run(['gcc',*flags,*sources,*libs,'-o',str(out/'pitonic-x86_64.so')],check=True)
root=Path(os.environ.get('PITONIC_SYSROOT',str(B.parent/'piClideaN MPC/toolchain/root')));cc=root/'usr/bin/arm-linux-gnueabihf-gcc-12';env=dict(os.environ);env['PATH']=str(root/'usr/bin')+os.pathsep+env.get('PATH','');env['LD_LIBRARY_PATH']=str(root/'usr/lib/x86_64-linux-gnu')+os.pathsep+env.get('LD_LIBRARY_PATH','')
subprocess.run([str(cc),'--sysroot='+str(root),'-march=armv7-a','-mfpu=neon','-mfloat-abi=hard',*flags,*sources,'-static-libgcc',*libs,'-o',str(out/'pitonic.so')],env=env,check=True)
inputs=[B/'params.json',B/'build/params.h',*list((B/'src').glob('*')),*list((B/'third_party/mpc-wrapper/wrapper').glob('*')),generated/'pitonic_factory_programs.h',Path(__file__)]
receipt={'private_only':private_only,'programs':len(items),'inputs':{str(f.relative_to(B)):hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs if f.is_file()},'outputs':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in out.glob('*.so')}}
(out/'BUILD.json').write_text(json.dumps(receipt,indent=2)+'\n');print('ARMv7 and PC plugins expose',len(items),'complete-state native VST programs.')
