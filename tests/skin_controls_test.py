import json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from config import B,FOLDER
from PIL import Image
p=json.loads((B/'params.json').read_text());indices={x['key']:i for i,x in enumerate(p)}
a=B/'build/skin'/FOLDER/'Plugin Skins'
expected=[[f'q0_{i}' for i in [0,1,2,3,5,6,7]],[f'q1_{i}' for i in [0,2,3,4,5]],[f'q2_{i}' for i in range(7)],['rev_size','rev_decay','rev_damp','rev_return','fx_time','del_feedback','del_tone','del_return'],[f'v{i}_level' for i in range(8)]]
for fn in ['Q-Links.json','Q-Links - 8by1.json','Q-Links - 4by4.json']:
 document=json.loads((a/fn).read_text())
 assert set(document)=={'version','info','Screen Mode Q-Links','Program Mode Q-Links'}
 assert document['Program Mode Q-Links']==document['Screen Mode Q-Links']['map'][0]['Q-Links']
 rows=document['Screen Mode Q-Links']['map']
 for row,keys in zip(rows,expected):
  mapping=row['Q-Links'];assert len(mapping)==16
  assert [mapping[f'Q-Link {k}'] for k in [13,9,5,1,14,10,6,2]]==[indices[k] for k in keys]+[-1]*(8-len(keys))
assert [x['key'] for x in p[293:301]]==[f'activity{i}' for i in range(8)]
defs=json.loads((a/'TUI.json').read_text())['pageData']['componentDefinitions']['localComponentDefinitions']
for d in defs:
 value=d['value']
 if value['actions']:
  focus=value['componentsData'][-1];assert focus['bounds']['whenVisible']=='WhenFocussed' and focus['bounds']['acceptsHWFocus']=='No'
  im=Image.open(a/focus['componentData']['data']['image']);assert im.mode=='RGBA' and im.getpixel((0,0))[3]==0
for v in range(8):
 im=Image.open(a/f'pad{v}_1.png');assert (241,242,237) not in im.getdata(), 'Permanent white selected border'
print('PASS: exact physical Q-Link ordering, empty slots, appended indices, passive native focus overlays and no permanent white selection outline')

assert len(p)==331
assert (B/'params.lock').read_text().splitlines()[:301]==(B/'tests/fixtures/parameter-prefix.lock').read_text().splitlines()
tabs=json.loads((a/'TUI.json').read_text())['pageData']['tabs']
assert [t['tabName'] for t in tabs]==['TONE','NOISE','CHARACTER','EFFECTS','MIXER']
print('PASS: five tabs, 301 unchanged indices and shared effects Q-Link page')

# EFFECTS uses rotary artwork; MIXER retains its horizontal sliders.
D={d['key']:d['value'] for d in defs}
def knob_strips(c):
 kind=c['componentData']['type']
 if kind=='Knob':return [c['componentData']['data']['filmStrip']]
 if kind in D:return [strip for child in D[kind]['componentsData'] for strip in knob_strips(child)]
 return []
fx=knob_strips({'componentData':{'type':tabs[3]['componentName']}})
mix=knob_strips({'componentData':{'type':tabs[4]['componentName']}})
assert len(fx)==8 and set(fx)=={'dial.png'},fx
assert len(mix)==32 and set(mix)=={'slider.png'},mix
print('PASS: eight EFFECTS rotary controls; MIXER keeps 32 sliders')
