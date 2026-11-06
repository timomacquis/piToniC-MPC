import json,sys,hashlib,shutil
from config import B,FIELDS,FX_FIELDS
sys.path.insert(0,str(B/'third_party/mpc-wrapper/tools'))
import gen_vst
P=[dict(key='selected',name='Voice',min=0,max=7,default=0,display='int',options=[f'A{i+1:02}' for i in range(8)])]
for v in range(8):
 for key,name,lo,hi,default,unit in FIELDS:
  p=dict(key=f'v{v}_{key}',name=f'A{v+1:02} {name}',min=lo,max=hi,default=default,dynamic_display=True,poll=False)
  if isinstance(unit,list):p.update(options=unit,display='int')
  P.append(p)
 for k in ['mute','solo']:P.append(dict(key=f'v{v}_{k}',name=f'A{v+1:02} {k.upper()}',min=0,max=1,default=0,options=['OFF','ON']))
P.extend(dict(key=k,name=k.upper(),min=0,max=1,default=0,momentary=True,hold_ms=120) for k in ['trigger','alter','random','compare'])
for v in range(8):P.append(dict(key=f'select{v}',name=f'A{v+1:02}',min=0,max=1,default=0,momentary=True))
for v in range(8):P.append(dict(key=f'ab{v}',name=f'A{v+1:02} B',min=0,max=1,default=0,options=['A','B'],poll=False))
for page in range(3):
 for i in range(8):
  f=FIELDS[page*8+i];p=dict(key=f'q{page}_{i}',name=f[1],min=f[2],max=f[3],default=f[4],dynamic_display=True,dynamic_name=True,poll=False)
  if isinstance(f[5],list):p.update(options=f[5],display='int')
  P.append(p)
for v in range(8):
 for field in FIELDS:
  if isinstance(field[5],list):
   key=f'v{v}_{field[0]}'
   P.append(dict(key=key+'__open',name='Options',min=0,max=1,default=0,popup_of=key))
for v in range(8):P.append(dict(key=f'activity{v}',name=f'A{v+1:02} activity',min=0,max=1,default=0,poll=False))
for v in range(8):
 for k in ['reverb','delay']:P.append(dict(key=f'send{v}_{k}',name=f'A{v+1:02} {k.upper()} SEND',min=0,max=100,default=0,dynamic_display=True,poll=False))
for key,name,lo,hi,default,unit in FX_FIELDS:
 p=dict(key=key,name=name,min=lo,max=hi,default=default,dynamic_display=True,poll=False)
 if isinstance(unit,list):p.update(options=unit,display='int')
 P.append(p)
P.append(dict(key='del_div__open',name='Delay time options',min=0,max=1,default=0,popup_of='del_div'))
P.append(dict(key='fx_time',name='DELAY TIME',min=0,max=1,default=.5,dynamic_display=True,poll=False))
keys=[p['key'] for p in P];lock=B/'params.lock'
if lock.exists():assert keys[:len(old:=lock.read_text().splitlines())]==old,'Parameter indices changed'
lock.write_text('\n'.join(keys)+'\n')
(B/'params.json').write_text(json.dumps(P,indent=2)+'\n');(B/'build').mkdir(exist_ok=True)
cfg=gen_vst.load(str(B/'vst.json'));gen_vst.gen_params(cfg,P,str(B/'build/params.h'))
with (B/'build/params.h').open('a') as f:
 f.write('\nstatic const unsigned char PICN_AUTOMATABLE[NPARAMS]={'+','.join('1' if (p['key'].startswith(('v','send','rev_','del_')) and '__open' not in p['key']) else '0' for p in P)+'};\n')
 f.write('static const unsigned short PICN_PARAM_LOOKUP[NPARAMS]={'+','.join(str(i) for i in sorted(range(len(P)),key=lambda i:keys[i]))+'};\n')
# Musical taper for Hz and time controls; canonical storage remains physical units.
curved={'freq','attack','decay','modrate','cutoff','natk','ndcy','eqfreq'}
flags=[]
for p in P:
 k=p['key'];field=k.split('_',1)[1] if k.startswith('v') and '__open' not in k else FIELDS[int(k[1])*8+int(k[3])][0] if k.startswith('q') else ''
 flags.append('1' if field in curved else '0')
with (B/'build/params.h').open('a') as f:f.write('static const unsigned char PITONIC_CURVE[NPARAMS]={'+','.join(flags)+'};\n')
fields=['/* Generated from scripts/config.py. */','#pragma once',f'#define PT_FIELDS {len(FIELDS)}','typedef struct {const char *key,*name,*unit;float lo,hi,def;int options;} PTField;','static const PTField pt_fields[PT_FIELDS]={']
for k,n,lo,hi,d,u in FIELDS:fields.append('{'+','.join([json.dumps(k),json.dumps(n),json.dumps('' if isinstance(u,list) else u),str(lo),str(hi),str(d),str(len(u) if isinstance(u,list) else 0)])+'},')
fields+=['};'];(B/'src/fields.h').write_text('\n'.join(fields)+'\n')
print('Generated',len(P),'stable parameters')
