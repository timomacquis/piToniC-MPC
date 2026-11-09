from pathlib import Path
import json,sys
from PIL import Image
B=Path(__file__).resolve().parents[1];sys.path.insert(0,str(B/'scripts'));from config import FOLDER
A=B/'build/skin'/FOLDER/'Plugin Skins';doc=json.loads((A/'TUI.json').read_text())['pageData'];defs={d['key']:d['value'] for d in doc['componentDefinitions']['localComponentDefinitions']};params=json.loads((B/'params.json').read_text());keys=[p['key'] for p in params];baseline=(B/'Validation/sample-1.0.1-candidate.2/baseline/params.lock').read_text().splitlines();assert len(keys)==418 and keys==baseline
page=defs[doc['tabs'][0]['componentName']]['componentsData']
def mapped(c):
 return [keys[int(x['value'].split()[-1])] for x in c.get('handle remapping',{}).get('map',[]) if x['key']=='Data' and x['value'].startswith('Parameter ')]
loads=[c for c in page if mapped(c)==['load']];assert len(loads)==8 and all(c['bounds']['bounds']=='486 135 128 42' for c in loads)
source=[c for c in page if c['componentData']['type']=='Image' and c['componentData']['data'].get('image')=='source_title.png'];assert len(source)==8 and all(c['bounds']['bounds']=='20 78 128 42' for c in source)
assert not any(set(mapped(c))&{'sample_info','sample_status'} for c in page)
for v in range(8):
 for mode in range(2):
  name=f'source_select{v}_{mode}';c=next(c for c in page if c['componentData']['type']==name);assert c['bounds']['bounds']==f'{350+mode*136} 78 128 42'
for n in ['source_title.png','source_SYNTH_off.png','source_SAMPLE_off.png','button_LOAD_128_42_0.png']:
 im=Image.open(A/n);assert im.size==(128,42) and im.getpixel((5,5))==Image.open(A/'source_SYNTH_off.png').getpixel((5,5))
for key,height in [('browse_path',24),('browse_page',24)]:
 found=[v for k,v in defs.items() if k.startswith('sample_read_'+key+'_')];assert len(found)==1
 assert found[0]['componentsData'][0]['componentData']['data']['textStyle']['font']['height']==height
for i in range(8):assert defs['sample_entry'+str(i)]['componentsData'][1]['componentData']['data']['textStyle']['font']['height']==26
strip=Image.open(A/'sample_wave.png').convert('RGB');assert strip.size==(18,80*128)
for frame in range(128):
 code=min(frame,120);negative,slot=divmod(code,11);positive=10-slot if negative&1 else slot;lo=40-round(36*positive/10);hi=40+round(36*negative/10);tile=strip.crop((0,frame*80,18,(frame+1)*80))
 for y in range(80):
  expected=(169,210,223) if lo<=y<=hi else (20,29,36)
  assert tile.getpixel((0,y))==tile.getpixel((17,y))==expected,(frame,y)
for v in range(8):
 for i in range(32):
  c=next(c for c in page if c['componentData']['type']==f'sample_peak{v}_{i}');assert c['bounds']['bounds']==f'{24+i*18} 186 18 80';knob=defs[f'sample_peak{v}_{i}']['componentsData'][0];assert knob['componentData']['data']['numFrames']==128 and knob['bounds']['bounds']=='0 0 18 80' and knob['bounds']['acceptsHWFocus']=='No'
print('PASS sample UI: matching SOURCE/SYNTH/SAMPLE/LOAD sizes, LOAD alignment, browser 24/26/24, hidden info/status, 128 complete signed-envelope frames, contiguous baseline-aligned columns and 418 unchanged indices')
