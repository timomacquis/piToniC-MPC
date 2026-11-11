"""Original native MPC skin; shared cached artwork, no stock skin redistribution."""
import json,math,hashlib,shutil,os,copy
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from config import B,FIELDS,FOLDER,VERSION
P=json.loads((B/'params.json').read_text());I={p['key']:i for i,p in enumerate(P)}
OUT=B/os.environ.get('PTON_SKIN_OUTPUT','build/skin')/FOLDER;A=OUT/'Plugin Skins'
if OUT.exists():shutil.rmtree(OUT)
A.mkdir(parents=True,exist_ok=True);defs=[]
BG='#171c1f';PANEL='#141d24';FIELD='#242c31';INK='#f1f2ed';EDGE='#42515c';AMBER='#c7aa6d'
def font(n):return ImageFont.truetype(str(B/'reference/fonts/TitilliumWeb-Regular.ttf'),n)
def bitmap(name,w,h,caption='',fill=FIELD,ink=INK):
 im=Image.new('RGB',(w,h),BG);d=ImageDraw.Draw(im);d.rounded_rectangle((1,1,w-2,h-2),radius=9,fill=fill,outline=EDGE);d.text((w/2,h/2),caption,font=font(19 if w<140 else 23),fill=ink,anchor='mm');im.save(A/name);return name
def bounds(x,y,w,h,extra=None):return dict(version=2,acceptsHWFocus='Yes',showWhenDataModelInvalid='Hide',whenVisible='Always',boundsType='Absolute',bounds=f'{x} {y} {w} {h}',additionalInvalidatingHandles=extra or [])
def child(kind,data,x,y,w,h):return {'version':2,'componentData':{'version':1,'name':kind,'type':kind,'data':data},'handle remapping':{'version':1,'map':[]},'bounds':bounds(x,y,w,h)}
def gate(key,v,n=2):return f'IndexedEnabling/{v}/{n}/Parameter {I[key]}'
def art(name,x,y,w,h):
 c=child('Image',dict(version=2,imageType='Regular',colour='0',image=name),x,y,w,h);c['bounds']['acceptsHWFocus']='No';return c
def label(x,y,w,h,kind='Value',n=26):
 c=child('Label',dict(version=1,textStyle=dict(version=1,font=dict(version=1,name='Titillium Web',style='Regular',height=float(n)),colour='fff1f2ed',justification='left verticallyCentred',case='Original'),type=kind,handleName='Data'),x,y,w,h);c['bounds']['acceptsHWFocus']='No';return c
def action(h,event='Mouse Down'):return {'version':2,'onAction':event,'handler':h,'handleName':'Data','additionalData':'','handle remapping':{'version':1,'map':[]}}
def local(name,parts,actions=None,focus_ring=True):
 if actions and focus_ring:
  for part in parts:part['bounds']['acceptsHWFocus']='No'
  w=max(int(c['bounds']['bounds'].split()[0])+int(c['bounds']['bounds'].split()[2]) for c in parts);h=max(int(c['bounds']['bounds'].split()[1])+int(c['bounds']['bounds'].split()[3]) for c in parts)
  fn=f'focus_{w}_{h}.png'
  if not (A/fn).exists():
   im=Image.new('RGBA',(w,h));ImageDraw.Draw(im).rounded_rectangle((2,2,w-3,h-3),radius=8,outline='#ffffff',width=1);im.save(A/fn)
  focus=art(fn,0,0,w,h);focus['bounds']['whenVisible']='WhenFocussed';parts.append(focus)
 clear=dict(version=1,colour='0',image='');defs.append(dict(key=name,value=dict(version=4,actions=actions or [],backgroundData=dict(version=1,focussed=clear,unfocussed=clear),ignoreMousePresses=not bool(actions),disableCoarseDataWheel=False,repeats=1,hideQLinkBounds=True,componentsData=parts)));return name
serial=0
def place(kind,key,x,y,w,h,extra=None):
 c=child(kind,dict(version=1,handleName='Data'),x,y,w,h);c['handle remapping']['map']=[dict(key='Data',value=f'Parameter {I[key]}')];c['bounds']['additionalInvalidatingHandles']=extra or [];return c
def toggle(key,title,x,y,w,h,extra=None):
 global serial
 serial+=1;parts=[]
 for state in [0,1]:
  fn=f'button_{title.replace("/","-").replace(" ","_")}_{w}_{h}_{state}.png'
  if not (A/fn).exists():bitmap(fn,w,h,('ON' if state else 'OFF') if key in ['rev_on','del_on'] else ('SYNC' if state else 'FREE') if key=='del_sync' else title,'#383122' if state else FIELD,AMBER if state else INK)
  c=art(fn,0,0,w,h);c['bounds']['additionalInvalidatingHandles']=[gate(key,state)];parts.append(c)
 name=local('toggle'+str(serial),parts,[action('Toggle Switch'),action('Toggle Switch','Enter Pressed')]);return place(name,key,x,y,w,h,extra)
def dial(key,title,x,y,w,h,extra=None,slider=False):
 global serial
 serial+=1;fn=f'box_{w}_{h}.png'
 if slider:
  fn=f'mixer_control_{w}_{h}.png';Image.new('RGB',(w,h),PANEL).save(A/fn)
 elif not (A/fn).exists():bitmap(fn,w,h,'',PANEL)
 parts=[art(fn,0,0,w,h)]
 if slider:
  sw=128 if w<200 else w-20;strip='slider.png' if sw==128 else f'slider_{sw}.png'
  if not (A/strip).exists():
   im=Image.new('RGB',(sw,26*128),PANEL);d=ImageDraw.Draw(im)
   for frame in range(128):
    yy=frame*26;xx=5+(sw-10)*frame/127;d.rounded_rectangle((2,yy+10,sw-3,yy+16),radius=3,fill='#55595d');d.line((5,yy+13,xx,yy+13),fill=AMBER,width=5);d.ellipse((xx-6,yy+7,xx+6,yy+19),fill=AMBER)
   im.save(A/strip)
  parts += [child('Knob',dict(version=5,knobType='FilmStrip',filmStrip=strip,numFrames=127,invert=False,dragOrientation='Horizontal',handleName='Data'),10,57,sw,26),label(12,29,w-24,27,n=25)]
 else:parts += [child('Knob',dict(version=5,knobType='FilmStrip',filmStrip='dial.png',numFrames=127,invert=False,dragOrientation='Vertical',handleName='Data'),12,(h-58)//2,58,58),label(86,43,w-94,42,n=30)]
 # Static captions are artwork, avoiding repeated host name queries.
 cap=f'caption_{hashlib.sha256(title.encode()).hexdigest()[:12]}_{w}.png'
 if not (A/cap).exists():
  im=Image.new('RGB',(w-16,25),PANEL);ImageDraw.Draw(im).text((0,0),title,font=font(18),fill=INK);im.save(A/cap)
 parts.append(art(cap,10 if slider else 86,8,w-16 if slider else w-94,25))
 if slider:
  for c in parts:
   if c['componentData']['type']=='Label':c['componentData']['data']['textStyle']['justification']='centred'
 return place(local('dial'+str(serial),parts,[action('Q-Link')]),key,x,y,w,h,extra)
# One shared dial and one shared slider strip, used by every page.
for slider in [False,True]:
 w,h=(128,26) if slider else (58,58);im=Image.new('RGB',(w,h*128),PANEL);d=ImageDraw.Draw(im)
 for f in range(128):
  y=f*h
  if slider:
   d.rounded_rectangle((2,y+10,w-3,y+16),radius=3,fill='#55595d');x=5+(w-10)*f/127;d.line((5,y+13,x,y+13),fill=AMBER,width=5);d.ellipse((x-6,y+7,x+6,y+19),fill=AMBER)
  else:
   d.ellipse((4,y+4,54,y+54),outline=INK,width=2);a=math.radians(-135+270*f/127);d.line((29,y+29,29+math.sin(a)*20,y+29-math.cos(a)*20),fill=INK,width=3)
 im.save(A/('slider.png' if slider else 'dial.png'))
Image.new('RGB',(1280,628),BG).save(A/'background.png')
def selectors(parts):
 for v in range(8):
  cs=[]
  for state in [0,1]:
   fn=f'pad{v}_{state}.png';bitmap(fn,148,56,f'A{v+1:02}', '#39262c' if state else FIELD,'#ee4b5d' if state else INK)
  # The base is shared with one conditional selected overlay: two images per pad.
  cs=[art(f'pad{v}_0.png',0,0,148,56)]
  c=art(f'pad{v}_1.png',0,0,148,56);c['bounds']['additionalInvalidatingHandles']=[gate('selected',v,8)];cs.append(c)
  fn=f'activity{v}.png';bitmap(fn,148,56,f'A{v+1:02}', '#203c49','#91c9df')
  c=art(fn,0,0,148,56);c['bounds']['additionalInvalidatingHandles']=[gate(f'activity{v}',1)];cs.append(c)
  fn=f'activity_selected{v}.png';bitmap(fn,148,56,f'A{v+1:02}', '#203c49','#ee4b5d')
  c=art(fn,0,0,148,56);c['bounds']['additionalInvalidatingHandles']=[gate(f'activity{v}',1),gate('selected',v,8)];cs.append(c)
  parts.append(place(local(f'selector{v}_{len(defs)}',cs,[action('Toggle Switch')]),f'select{v}',20+v*156,10,148,56))
def tools(parts,mixer):
 for v in range(8):
  cond=[gate('selected',v,8)]
  if not mixer:
   for j,(key,title) in enumerate([(f'v{v}_mute','MUTE'),(f'v{v}_solo','SOLO'),('trigger','TRIGGER')]):parts.append(toggle(key,title,20+j*156,548,148,52,cond))
  parts.append(toggle('compare','A / B',900,548,110,52,cond));parts.append(toggle('alter','ALTER',1020,548,90,52,cond));parts.append(toggle('random','RAND VOICE',1120,548,140,52,cond))
  # A/B readout is tied to this voice, independent of the momentary switch.
  parts.append(place(local(f'abread{v}_{len(defs)}',[label(0,0,48,28,n=20)]),f'ab{v}',950,604,48,24,cond))
# LCD: one shared 256-frame atlas, 15 x 15 bipolar peak states.
# 2 x 52 x 256 stays below the 16384-pixel filmstrip height ceiling.
LCD_COLS=288
im=Image.new('RGB',(2,52*256),'#020405');d=ImageDraw.Draw(im)
for frame in range(256):
 code=round(frame*224/255);negative,slot=divmod(code,15);positive=14-slot if negative&1 else slot;yy=frame*52+26
 d.line((0,yy,1,yy),fill='#29434b')
 if code:d.rectangle((0,yy-round(23*positive/14),1,yy+round(23*negative/14)),fill='#badde8')
im.save(A/'sample_lcd.png')
im=Image.new('RGB',(596,136),'#020405');ImageDraw.Draw(im).rectangle((0,0,595,135),outline='#42515c');im.save(A/'sample_lcd_frame.png')
im=Image.new('RGBA',(2,120));d=ImageDraw.Draw(im)
for yy in range(0,120,9):d.line((0,yy,0,min(yy+4,119)),fill=INK,width=1)
im.save(A/'sample_start_marker.png')
def sample_lcd(parts):
 sam=[gate('source',1)]
 c=art('sample_lcd_frame.png',20,180,596,136);c['bounds']['additionalInvalidatingHandles']=sam;parts.append(c)
 for channel,stereo,y in [(0,0,222),(0,1,188),(1,1,254)]:
  for i in range(LCD_COLS):
   knob=child('Knob',dict(version=5,knobType='FilmStrip',filmStrip='sample_lcd.png',numFrames=256,invert=False,dragOrientation='Vertical',handleName='Data'),0,0,2,52);knob['bounds']['acceptsHWFocus']='No'
   c=place(local(f'lcd_{channel}_{stereo}_{i}',[knob]),f'sample_lcd{channel*LCD_COLS+i}',28+i*2,y,2,52,sam+[gate('sample_stereo',stereo)]);c['bounds']['acceptsHWFocus']='No';parts.append(c)
 # START changes only these tiny cached markers, not the waveform handles.
 for i in range(LCD_COLS):
  c=art('sample_start_marker.png',28+i*2,188,2,120);c['bounds']['additionalInvalidatingHandles']=sam+[gate('sample_marker',i,LCD_COLS)];parts.append(c)
def readout(key,x,y,w,h,n=20):return place(local('sample_read_'+key+'_'+str(len(defs)),[label(0,0,w,h,n=n)]),key,x,y,w,h)
def tone_sample(parts,v):
 selected=[gate('selected',v,8)];syn=selected+[gate(f'v{v}_source',0)];sam=selected+[gate(f'v{v}_source',1)]
 bitmap('source_title.png',128,42,'SOURCE');parts.append(art('source_title.png',20,78,128,42));parts[-1]['bounds']['additionalInvalidatingHandles']=selected
 # Two native radio buttons on one binary parameter.
 for mode,title in enumerate(['SYNTH','SAMPLE']):
  off=bitmap('source_'+title+'_off.png',128,42,title);on=bitmap('source_'+title+'_on.png',128,42,title,'#383122',AMBER)
  native=child('Button',dict(version=2,onImage=on,offImage=off,buttonId=mode,numButtonsInGroup=2,handleName='Data',gestureBehaviour='Instant'),0,0,128,42)
  parts.append(place(local(f'source_select{v}_{mode}',[native],[action('Q-Link')]),f'v{v}_source',350+mode*136,78,128,42,selected))
 parts.append(dial('tone_q0','FREQUENCY',20,138,292,122,syn));parts.append(dial('tone_q1','WAVE MORPH',324,138,292,122,syn))
 for key,title,x in [('tone_q0','TUNE',20),('tone_q1','START',324)]:parts.append(dial(key,title,x,336,292,110,sam))
 for key,x,y,w,h,n in [('sample_name',30,132,440,36,23)]:parts.append(readout(key,x,y,w,h,n));parts[-1]['bounds']['additionalInvalidatingHandles']=sam
 parts.append(toggle('load','LOAD',486,135,128,30,sam))
 parts.extend([dial('tone_q2','AMP ATTACK',648,138,300,122,selected),dial('tone_q3','AMP DECAY',960,138,300,122,selected)])
 parts.append(toggle(f'v{v}_modmode__open','PITCH MODE',648,280,300,42,selected));parts.append(readout(f'v{v}_modmode',972,280,280,42,24));parts[-1]['bounds']['additionalInvalidatingHandles']=selected
 for key,title,x in [('tone_q4','PITCH AMOUNT',648),('tone_q5','PITCH TIME / RATE',960)]:parts.append(dial(key,title,x,336,300,110,selected))
 parts.append(dial('tone_q6','PITCH VELOCITY',960,450,300,86,selected))

# Compact selection panels; first-level widgets are kept byte-for-byte intact.
compact_layout=[]
def modal_base(parts,key,title,options,flag,cond,close_key=None,width=None,height=None,caption=None):
 opener=next(c for c in parts if any(m['key']=='Data' and m['value']==f'Parameter {I[key]}' for m in c['handle remapping']['map']))
 xx,yy,ow,oh=map(int,opener['bounds']['bounds'].split())
 n=len(options);cols=3 if n in [3,5] else min(2,n);rows=math.ceil(n/cols)
 bw=max(110,math.ceil(max([font(22).getlength(t) for t in options] or [100]))+32)
 w=width or max(420,48+cols*bw+(cols-1)*12);bw=(w-48-(cols-1)*12)//cols;footer=70+rows*36+(rows-1)*12+22;h=height or footer+60
 x=(1280-w)//2;y=(628-h)//2
 if x<xx+ow and x+w>xx and y<yy+oh and y+h>yy:
  if yy-h-12>=0:y=yy-h-12
  elif yy+oh+12+h<=628:y=yy+oh+12
  elif xx-w-12>=0:x=xx-w-12
  elif xx+ow+12+w<=1280:x=xx+ow+12
 close_key=close_key or flag
 if not (A/'compact_scrim.png').exists():Image.new('RGBA',(1280,628),(4,7,13,155)).save(A/'compact_scrim.png')
 blocker=local('compact_blocker_'+str(len(defs)),[art('compact_scrim.png',0,0,1280,628)],[action('Toggle Switch')],focus_ring=False)
 cs=[place(blocker,close_key,0,0,1280,628)]
 fn=f'compact_opener_{len(defs)}.png';bitmap(fn,ow,oh,caption or '', '#383122',AMBER)
 if not caption:
  im=Image.open(A/fn);ImageDraw.Draw(im).text((10,8),'TIME',font=font(18),fill=INK);im.save(A/fn)
 raised=[art(fn,0,0,ow,oh)]
 if not caption:raised.append(label(20,40,ow-40,52,n=30))
 raised_type=local('compact_raised_'+str(len(defs)),raised);c=place(raised_type,'del_div' if not caption else key,xx,yy,ow,oh);c['bounds']['acceptsHWFocus']='No';cs.append(c)
 panel=f'compact_panel_{w}_{h}.png';bitmap(panel,w,h,'',PANEL)
 surface=local('compact_surface_'+str(len(defs)),[art(panel,0,0,w,h)],[action('Q-Link')],focus_ring=False);cs.append(place(surface,close_key,x,y,w,h))
 fn='compact_title_'+hashlib.sha256(title.encode()).hexdigest()[:12]+'.png';im=Image.new('RGB',(w-48,32),PANEL);ImageDraw.Draw(im).text((0,16),title,font=font(22),fill=INK,anchor='lm');im.save(A/fn);cs.append(art(fn,x+24,y+14,w-48,32))
 compact_layout.append(dict(key=key,flag=flag,title=title,panel=[x,y,w,h],opener=[xx,yy,ow,oh],columns=cols,options=n,conditions=cond))
 return cs,x,y,w,h,bw,cols,footer
def finish_modal(parts,cs,cond):
 for c in cs:c['bounds']['additionalInvalidatingHandles'].extend(cond);parts.append(c)
def compact_picker(parts,key,title,options,cond):
 flag=key+'__open';cs,x,y,w,h,bw,cols,footer=modal_base(parts,flag,title,options,flag,cond,caption=None if key=='del_div' else title)
 for op,caption in enumerate(options):
  tag='compact_choice_'+hashlib.sha256((key+str(op)).encode()).hexdigest()[:14]
  for state in [0,1]:bitmap(tag+f'_{state}.png',bw,36,caption,'#383122' if state else FIELD,AMBER if state else INK)
  native=child('Button',dict(version=2,onImage=tag+'_1.png',offImage=tag+'_0.png',buttonId=op,numButtonsInGroup=len(options),handleName='Data',gestureBehaviour='Instant'),0,0,bw,36)
  cs.append(place(local(tag,[native],[action('Q-Link')]),key,x+24+(op%cols)*(bw+12),y+70+(op//cols)*48,bw,36))
 cs.append(toggle(flag,'CLOSE',x+(w-160)//2,y+footer,160,36));finish_modal(parts,cs,cond)

tabs=[]
for page,title in enumerate(['TONE','NOISE','CHARACTER','EFFECTS','MIXER']):
 parts=[art('background.png',0,0,1280,628)];selectors(parts)
 if page==0:
  for v in range(8):tone_sample(parts,v)
  sample_lcd(parts)
 elif page<3:
  for v in range(8):
   for i in range(8):
    f=FIELDS[page*8+i];key=f'v{v}_{f[0]}';x=20+(i%4)*312;y=114+(i//4)*166;cond=[gate('selected',v,8)]
    if isinstance(f[5],list):
     parts.append(toggle(key+'__open',f[1],x,y,304,42,cond));parts.append(place(local(f'enumread_{page}_{v}_{i}',[label(0,0,280,50,n=30)]),key,x+12,y+46,280,50,cond))
    else:parts.append(dial(key,f[1],x,y,304,122,cond))
 elif page==4:
  for v in range(8):
   x=20+v*156;bitmap(f'strip{v}.png',148,456,'',PANEL);parts.append(art(f'strip{v}.png',x,80,148,456))
   parts.extend([dial(f'v{v}_level','LEVEL',x+3,84,142,90,slider=True),dial(f'v{v}_pan','PAN',x+3,182,142,90,slider=True),dial(f'send{v}_reverb','REVERB',x+3,280,142,90,slider=True),dial(f'send{v}_delay','DELAY',x+3,378,142,90,slider=True),toggle(f'v{v}_mute','M',x+10,480,59,44),toggle(f'v{v}_solo','S',x+79,480,59,44)])
 else:
  # Shared effects: each side owns four controls, with one central separator.
  im=Image.new('RGB',(2,420),EDGE);im.save(A/'effects_divider.png');parts.append(art('effects_divider.png',639,92,2,420))
  for heading,x in [('REVERB',24),('DELAY',660)]:
   fn=f'fx_title_{heading}.png';im=Image.new('RGB',(280,40),BG);ImageDraw.Draw(im).text((0,0),heading,font=font(28),fill=INK);im.save(A/fn);parts.append(art(fn,x,88,280,40))
  parts.extend([toggle('rev_on','ON',480,84,128,48),toggle('del_on','ON',1112,84,128,48),toggle('del_sync','SYNC',812,84,128,48)])
  for key,caption,x,y in [('rev_size','SIZE',24,154),('rev_decay','DECAY',336,154),('rev_damp','DAMPING',24,334),('rev_return','RETURN',336,334),('del_feedback','FEEDBACK',972,154),('del_tone','TONE',660,334),('del_return','RETURN',972,334)]:parts.append(dial(key,caption,x,y,284,122))
  parts.append(dial('del_ms','TIME / MS',660,154,284,122,[gate('del_sync',0)]))
  bitmap('delay_time_field.png',284,122,'',PANEL)
  im=Image.new('RGB',(264,25),PANEL);ImageDraw.Draw(im).text((0,0),'TIME',font=font(18),fill=INK);im.save(A/'delay_time_caption.png')
  content=[art('delay_time_field.png',0,0,284,122),label(20,40,244,52,n=30)]
  # Parameter label reads the division while parent exclusively owns opening the picker.
  read=place(local('delay_div_read',[label(20,40,244,52,n=30)]),'del_div',0,0,284,122)
  parts.append(place(local('delay_div_open',[content[0],art('delay_time_caption.png',10,8,264,25),read],[action('Toggle Switch')]),'del_div__open',660,154,284,122,[gate('del_sync',1)]))
 if page!=3:tools(parts,page==4)
 if page==3:
  compact_picker(parts,'del_div','DELAY TIME',['1/16','1/8','1/8 D','1/4','1/2'],[gate('del_div__open',1)])
 if page<3:
  for v in range(8):
   for f in FIELDS[page*8:page*8+8]:
    if isinstance(f[5],list):compact_picker(parts,f'v{v}_{f[0]}',f[1],f[5],[gate('selected',v,8),gate(f'v{v}_{f[0]}__open',1)])
 if page==0:
  cond=[gate('browse',1)]
  cs,x,y,w,h,_,_,_=modal_base(parts,'load','LOAD SAMPLE',[''], 'browse',cond,close_key='browse_close',width=1180,height=418,caption='LOAD')
  compact_layout[-1].update(kind='sample_browser',columns=2,options=8)
  cs.append(readout('browse_path',x+24,y+100,w-48,36,24))
  for key,caption,dx,bw in [('browse_up','UP',24,120),('browse_sd','USER / SD',156,180),('browse_media','DRIVES',348,180),('browse_close','CLOSE',w-184,160)]:cs.append(toggle(key,caption,x+dx,y+54,bw,36))
  for i in range(8):
   key=f'browse_item{i}';fn='compact_sample_entry.png';bitmap(fn,560,36,'',FIELD)
   cs.append(place(local('compact_sample_entry'+str(i),[art(fn,0,0,560,36),label(12,2,536,32,n=26)],[action('Toggle Switch')]),key,x+24+(i%2)*572,y+148+(i//2)*48,560,36))
  cs.append(toggle('browse_prev','PREVIOUS',x+24,y+354,180,36));cs.append(readout('browse_page',x+(w-260)//2,y+354,260,36,24));cs.append(toggle('browse_next','NEXT',x+w-204,y+354,180,36));finish_modal(parts,cs,cond)

 name=local('page'+str(page),parts);tabs.append(dict(version=3,tabName=title,fnKeyIndex=page,fnKeySubIndex=0,qlinkBoundsData=['0 0 0 0'],componentName=name,initialSize='0 0 1280 628',scale=1.))
J={'pageData':{'version':1,'info':{'version':1,'type':'CompleteDescription'},'componentDefinitions':{'version':1,'importFiles':['/usr/share/Akai/Content/Synths/Generic/Generic Knob Overlay.json','/usr/share/Akai/Content/Synths/Generic/Generic Menu Overlay.json'],'localComponentDefinitions':defs},'tabs':tabs}}
(B/'Validation/UI-Compact-Panels/layout.json').write_text(json.dumps(compact_layout,ensure_ascii=False,indent=2)+'\n')
(A/'TUI.json').write_text(json.dumps(J,ensure_ascii=False,separators=(',',':')))
ql=[]
page_keys=[[f'tone_q{i}' for i in range(7)], [f'q1_{i}' for i in [0,2,3,4,5]], [f'q2_{i}' for i in range(7)], ['rev_size','rev_decay','rev_damp','rev_return','fx_time','del_feedback','del_tone','del_return'], [f'v{i}_level' for i in range(8)]]
for page,keys in enumerate(page_keys):
 mapping={f'Q-Link {i}':-1 for i in range(1,17)}
 for physical,key in zip([13,9,5,1,14,10,6,2],keys):mapping[f'Q-Link {physical}']=I[key]
 ql.append({'Tab':page+1,'SubTab':1,'Bank Direction':'Column','Q-Links':mapping})
for name in ['Q-Links.json','Q-Links - 8by1.json','Q-Links - 4by4.json']:(A/name).write_text(json.dumps({'version':4,'info':{'version':1,'type':'CompleteDescription'},'Screen Mode Q-Links':{'version':4,'map':ql},'Program Mode Q-Links':ql[0]['Q-Links']},indent=2))
(OUT/'version.xml').write_text(f'<plugincontent version="1.0"><identifier>Timo.vst.πtonic</identifier><version>{VERSION.split("-")[0]}.0</version></plugincontent>\n')
print('Generated five native pages:',len(defs),'components;',len(list(A.glob('*.png'))),'cached images')
