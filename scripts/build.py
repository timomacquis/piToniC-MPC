"""Reproducible local build. ARM toolchain may be supplied with PITONIC_SYSROOT."""
import os,sys,subprocess,json,hashlib,shutil,platform
from config import B
os.chdir(B)
def run(args,**kw):subprocess.run([str(a) for a in args],check=True,**kw)
run([sys.executable,'scripts/generate.py'])
flags=['-std=c11','-O2','-fPIC','-shared','-Ibuild','-Isrc','-Ithird_party/mpc-wrapper/wrapper']
sources=['src/engine.c','third_party/mpc-wrapper/wrapper/vst2_wrap.c']
run(['gcc',*flags,*sources,'-lpthread','-lm','-ldl','-o','build/pitonic-x86_64.so'])
root=os.environ.get('PITONIC_SYSROOT')
if not root:root=str(B.parent/'piClideaN MPC/toolchain/root')
from pathlib import Path
root=Path(root);cc=root/'usr/bin/arm-linux-gnueabihf-gcc-12'
if not cc.exists():raise SystemExit('Set PITONIC_SYSROOT to the ARMv7 compiler/sysroot before building the device plugin.')
env=dict(os.environ);env['PATH']=str(root/'usr/bin')+os.pathsep+env.get('PATH','');env['LD_LIBRARY_PATH']=str(root/'usr/lib/x86_64-linux-gnu')+os.pathsep+env.get('LD_LIBRARY_PATH','')
run([cc,'--sysroot='+str(root),'-march=armv7-a','-mfpu=neon','-mfloat-abi=hard',*flags,*sources,'-static-libgcc','-lpthread','-lm','-ldl','-o','build/pitonic.so'],env=env)
receipt={'platform':platform.platform(),'compiler':subprocess.check_output([str(cc),'--version'],env=env,text=True).splitlines()[0],'inputs':{},'outputs':{}}
for f in [B/'vst.json',B/'params.json',*list((B/'src').glob('*')),*list((B/'third_party/mpc-wrapper/wrapper').glob('*')),*list((B/'third_party/mpc-wrapper/tools').glob('*.py')),*list((B/'scripts').glob('*.py')),*list((B/'tests').glob('*.c'))]:
 if f.is_file():receipt['inputs'][str(f.relative_to(B))]=hashlib.sha256(f.read_bytes()).hexdigest()
for f in [B/'build/pitonic.so',B/'build/pitonic-x86_64.so']:receipt['outputs'][f.name]=hashlib.sha256(f.read_bytes()).hexdigest()
receipt['skin']={str(f.relative_to(B/'build/skin')):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted((B/'build/skin').rglob('*')) if f.is_file()}
(B/'build/build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ARMv7 and local host binaries built; no device installation performed.')
