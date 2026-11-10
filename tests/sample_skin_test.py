from pathlib import Path
import json,sys
from PIL import Image
B=Path(__file__).resolve().parents[1];sys.path.insert(0,str(B/'scripts'));from config import FOLDER
A=B/'build/skin'/FOLDER/'Plugin Skins';doc=json.loads((A/'TUI.json').read_text())['pageData'];defs={d['key']:d['value'] for d in doc['componentDefinitions']['localComponentDefinitions']};params=json.loads((B/'params.json').read_text());keys=[p['key'] for p in params];baseline=(B/'Validation/sample-1.0.1-candidate.2/baseline/params.lock').read_text().splitlines();assert len(keys)==996 and keys[:418]==baseline
page=defs[doc['tabs'][0]['componentName']]['componentsData']
def mapped(c):
 return [keys[int(x['value'].split()[-1])] for x in c.get('handle remapping',{}).get('map',[]) if x['key']=='Data' and x['value'].startswith('Parameter ')]
loads=[c for c in page if mapped(c)==['load']];assert len(loads)==8 and all(c['bounds']['bounds']=='486 135 128 30' for c in loads)
source=[c for c in page if c['componentData']['type']=='Image' and c['componentData']['data'].get('image')=='source_title.png'];assert len(source)==8 and all(c['bounds']['bounds']=='20 78 128 42' for c in source)
assert not any(set(mapped(c))&{'sample_info','sample_status'} for c in page)
for v in range(8):
 for mode in range(2):
  name=f'source_select{v}_{mode}';c=next(c for c in page if c['componentData']['type']==name);assert c['bounds']['bounds']==f'{350+mode*136} 78 128 42'
for n in ['source_title.png','source_SYNTH_off.png','source_SAMPLE_off.png','button_LOAD_128_30_0.png']:
 im=Image.open(A/n);assert im.size==((128,30) if 'LOAD' in n else (128,42)) and im.getpixel((5,5))==Image.open(A/'source_SYNTH_off.png').getpixel((5,5))
for key,height in [('browse_path',24),('browse_page',24)]:
 found=[v for k,v in defs.items() if k.startswith('sample_read_'+key+'_')];assert len(found)==1
 assert found[0]['componentsData'][0]['componentData']['data']['textStyle']['font']['height']==height
for i in range(8):assert defs['sample_entry'+str(i)]['componentsData'][1]['componentData']['data']['textStyle']['font']['height']==26
strip=Image.open(A/'sample_lcd.png').convert('RGB');assert strip.size==(2,52*256)
assert max(Image.open(path).height for path in A.glob('*.png'))<=16384
for frame in range(256):
 code=round(frame*224/255);negative,slot=divmod(code,15);positive=14-slot if negative&1 else slot;lo=26-round(23*positive/14);hi=26+round(23*negative/14)
 for y in range(52):
  expected=(186,221,232) if code and lo<=y<=hi else (41,67,75) if y==26 else (2,4,5)
  assert strip.getpixel((0,frame*52+y))==strip.getpixel((1,frame*52+y))==expected
lcd=[c for c in page if c['componentData']['type'].startswith('lcd_')];assert len(lcd)==864
for c in lcd:
 assert c['bounds']['acceptsHWFocus']=='No'
 assert defs[c['componentData']['type']]['componentsData'][0]['componentData']['data']['filmStrip']=='sample_lcd.png'
markers=[c for c in page if c['componentData']['data'].get('image')=='sample_start_marker.png'];assert len(markers)==288
assert all(c['bounds']['acceptsHWFocus']=='No' and any(h.endswith('Parameter '+str(keys.index('sample_marker'))) for h in c['bounds']['additionalInvalidatingHandles']) for c in markers)
# Physical Screen/Program Q-Links and all visible TONE controls must bind the same aliases.
for slot in range(7):
 controls=[c for c in page if mapped(c)==['tone_q'+str(slot)]]
 assert len(controls)==(16 if slot<2 else 8),(slot,len(controls))
assert not any(set(mapped(c))&{'v0_freq','v0_wave','sample_tune','sample_marker'} and c['componentData']['type']!='Image' for c in page)
print('PASS LCD skin: 418 preserved indices; 288 columns/channel, shared cache below 16384 px, mono/stereo gates, independent START markers and exact tactile/physical alias identity')
