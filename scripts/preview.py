"""Offline layout proof from generated TUI components, not a device screenshot."""
import json,re,math
from PIL import Image,ImageDraw,ImageFont
from config import B,FIELDS,FOLDER,FX_FIELDS
P=json.loads((B/'params.json').read_text());A=B/'build/skin'/FOLDER/'Plugin Skins';J=json.loads((A/'TUI.json').read_text())['pageData'];D={d['key']:d['value'] for d in J['componentDefinitions']['localComponentDefinitions']};font=lambda n:ImageFont.truetype(str(B/'reference/fonts/TitilliumWeb-Regular.ttf'),int(n));cache={}
def asset(n):
 if n not in cache:cache[n]=Image.open(A/n).convert('RGB')
 return cache[n]
def visible(extra):
 for s in extra:
  m=re.fullmatch(r'IndexedEnabling/(\d+)/(\d+)/Parameter (\d+)',s);assert m,s
  v,n,i=map(int,m.groups());p=P[i];index=round((p['default']-p['min'])/(p['max']-p['min'])*(n-1))
  if index!=v:return False
 return True
def value(i):
 p=P[i];x=p['default'];k=p['key']
 if 'options' in p:return p['options'][round(x-p['min'])]
 for f in FX_FIELDS+FIELDS:
  if k==f[0] or k.endswith('_'+f[0]):return f'{x:g} ms' if f[0]=='modrate' else f'{x:g} {f[5]}' if isinstance(f[5],str) else str(x)
 return str(x)
def draw(c,im,ox=0,oy=0,parent=None):
 b=c['bounds'];x,y,w,h=map(int,b['bounds'].split());x+=ox;y+=oy
 if b.get('whenVisible')=='WhenFocussed' or not visible(b.get('additionalInvalidatingHandles',[])):return
 cd=c['componentData'];kind=cd['type'];data=cd['data'];maps=c.get('handle remapping',{}).get('map',[]);idx=parent
 for m in maps:
  if m['key']=='Data':idx=int(m['value'].split()[-1])
 if kind in D:
  for z in D[kind]['componentsData']:draw(z,im,x,y,idx)
 elif kind=='Image':im.paste(asset(data['image']).crop((0,0,w,h)),(x,y))
 elif kind=='Knob':
  p=P[idx];field=p['key'].split('_',1)[-1];off=1 if p['min']==0 else 0;normalized=math.log((p['default']+off)/(p['min']+off))/math.log((p['max']+off)/(p['min']+off)) if field in {'freq','attack','decay','modrate','cutoff','natk','ndcy','eqfreq'} else (p['default']-p['min'])/(p['max']-p['min']);frame=round(normalized*127);im.paste(asset(data['filmStrip']).crop((0,frame*h,w,(frame+1)*h)),(x,y))
 elif kind=='Label':
  centered=data['textStyle']['justification']=='centred';ImageDraw.Draw(im).text((x+w/2 if centered else x,y+h/2),value(idx),font=font(data['textStyle']['font']['height']),fill='#f1f2ed',anchor='mm' if centered else 'lm')
 elif kind=='Button':im.paste(asset(data['offImage']).crop((0,0,w,h)),(x,y))
 else:raise AssertionError(kind)
for i,t in enumerate(J['tabs']):
 im=Image.new('RGB',(1280,800),'#171c1f');d=ImageDraw.Draw(im)
 for x,title,text in [(24,'TRACK','πToniC'),(450,'PLUGIN','πToniC'),(900,'PRESET','Default')]:d.text((x,15),title,font=font(16),fill='#b4bcba');d.text((x,45),text,font=font(28),fill='#f1f2ed')
 d.line((20,96,1260,96),fill='#8c3548')
 for c in D[t['componentName']]['componentsData']:draw(c,im,0,108)
 for j,name in enumerate(['TONE','NOISE','CHARACTER','EFFECTS','MIXER']):
  x=j*256;d.line((x+20,740,x+236,740),fill='#b62650' if i==j else '#394953',width=3);d.text((x+128,766),name,font=font(23),fill='#f1f2ed',anchor='mm')
 out=B/'build/previews';out.mkdir(exist_ok=True);im.save(out/(t['tabName'].lower()+'.png'))
print('Five offline previews generated directly from the native component tree.')
