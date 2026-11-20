"""Reproducible local build. ARM toolchain may be supplied with PITONIC_SYSROOT."""
import os,sys,subprocess,json,hashlib,shutil,platform
from config import B
os.chdir(B)
def run(args,**kw):subprocess.run([str(a) for a in args],check=True,**kw)
run([sys.executable,'scripts/generate.py'])
flags=['-std=c11','-D_POSIX_C_SOURCE=200809L','-O2','-fPIC','-shared','-Ibuild','-Isrc','-Ithird_party/mpc-wrapper/wrapper']
sources=['src/engine.c','third_party/mpc-wrapper/wrapper/vst2_wrap.c']
cxx=['-std=c++17','-O2','-fPIC','-fno-exceptions','-fno-rtti','-Isrc']
run(['g++',*cxx,'-c','src/fx_new.cpp','-o','build/fx_new-x86_64.o'])
cflags=[f for f in flags if f!='-shared']
objs=lambda tag,cc_,extra,e=None:[run([cc_,*extra,*cflags,'-c',src,'-o',f'build/{Path(src).stem}-{tag}.o'],env=e) or f'build/{Path(src).stem}-{tag}.o' for src in sources]
from pathlib import Path
run(['g++','-shared',*objs('x86_64','gcc',[]),'build/fx_new-x86_64.o','-lpthread','-lm','-ldl','-o','build/pitonic-x86_64.so'])
root=os.environ.get('PITONIC_SYSROOT')
if not root:root=str(B.parent/'πClideaN/toolchain/root')
from pathlib import Path
root=Path(root);cc=root/'usr/bin/arm-linux-gnueabihf-gcc-12';cxxc=root/'usr/bin/arm-linux-gnueabihf-g++-12'
if not cc.exists():raise SystemExit('Set PITONIC_SYSROOT to the ARMv7 compiler/sysroot before building the device plugin.')
env=dict(os.environ);env['PATH']=str(root/'usr/bin')+os.pathsep+env.get('PATH','');env['LD_LIBRARY_PATH']=str(root/'usr/lib/x86_64-linux-gnu')+os.pathsep+env.get('LD_LIBRARY_PATH','')
# 1.1.0 : comme πMorpH 0.2.0 (testé sur Force), édition de liens contre la sysroot Gen1 (glibc et libstdc++ 10 de la Force).
old=Path(os.environ.get('PITONIC_GEN1_SYSROOT',B.parent/'πMorpH/build/gen1-sysroot'));lib=old/'usr/arm-linux-gnueabihf/lib';stdlib=old/'usr/lib/gcc-cross/arm-linux-gnueabihf/10'
if not (old/'usr/arm-linux-gnueabihf/include/c++/10').is_dir():raise SystemExit('Sysroot Gen1 introuvable : '+str(old))
arm=['--sysroot='+str(old),'-isystem',str(old/'usr/arm-linux-gnueabihf/include'),'-B'+str(lib)+'/','-L'+str(lib),'-L'+str(stdlib),'-march=armv7-a','-mfpu=neon','-mfloat-abi=hard']
cpp10=['-nostdinc++','-isystem',str(old/'usr/arm-linux-gnueabihf/include/c++/10'),'-isystem',str(old/'usr/arm-linux-gnueabihf/include/c++/10/arm-linux-gnueabihf')]
run([cxxc,*cpp10,*arm,*cxx,'-D_POSIX_C_SOURCE=200809L','-c','src/fx_new.cpp','-o','build/fx_new-armv7.o'],env=env)
run([cxxc,*arm,'-shared',*objs('armv7',cc,arm,env),'build/fx_new-armv7.o','-static-libstdc++','-static-libgcc','-lpthread','-lm','-ldl','-o','build/pitonic.so'],env=env)
# Contrôle : aucune version de glibc plus récente que celles déjà demandées par πToniC 1.0.4 (installé et validé sur la Force).
import zipfile,re
def glibc(data):return {tuple(map(int,v.split(b'.'))) for v in re.findall(rb'GLIBC_([0-9]+(?:\.[0-9]+)+)',data)}
with zipfile.ZipFile(B/'dist/1.0.4/piToniC-1.0.4-mpc-armv7.zip') as z:ref=glibc(z.read(next(n for n in z.namelist() if n.endswith('pitonic.so'))))
new=glibc((B/'build/pitonic.so').read_bytes());assert ref and max(new)<=max(ref),('glibc trop récente',sorted(new),max(ref))
print('glibc demandée : max',max(new),'(1.0.4 : max',max(ref),')')
receipt={'platform':platform.platform(),'compiler':subprocess.check_output([str(cc),'--version'],env=env,text=True).splitlines()[0],'inputs':{},'outputs':{}}
for f in [B/'vst.json',B/'params.json',*list((B/'src').glob('*')),*list((B/'third_party/mpc-wrapper/wrapper').glob('*')),*list((B/'third_party/mpc-wrapper/tools').glob('*.py')),*list((B/'scripts').glob('*.py')),*list((B/'tests').glob('*.c'))]:
 if f.is_file():receipt['inputs'][str(f.relative_to(B))]=hashlib.sha256(f.read_bytes()).hexdigest()
for f in [B/'build/pitonic.so',B/'build/pitonic-x86_64.so']:receipt['outputs'][f.name]=hashlib.sha256(f.read_bytes()).hexdigest()
receipt['skin']={str(f.relative_to(B/'build/skin')):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted((B/'build/skin').rglob('*')) if f.is_file()}
(B/'build/build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ARMv7 and local host binaries built; no device installation performed.')
