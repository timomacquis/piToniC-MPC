"""πToniC 1.2.0 : skin WAVETABLE (à lancer après scripts/build_skin.py)."""
import json,sys,re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from config import B,FOLDER,WT_TABLES
from PIL import Image
P=json.loads((B/'params.json').read_text());I={p['key']:i for i,p in enumerate(P)}
A=B/'build/skin'/FOLDER/'Plugin Skins';J=json.loads((A/'TUI.json').read_text())['pageData']
assert [t['tabName'] for t in J['tabs']]==['TONE','NOISE','CHARACTER','EFFECTS','MIXER']
D={d['key']:d['value'] for d in J['componentDefinitions']['localComponentDefinitions']}
assert len(D)==len(J['componentDefinitions']['localComponentDefinitions']),'noms de composants en double'
page=D[J['tabs'][0]['componentName']]['componentsData']
def param(c):return next((int(m['value'].split()[-1]) for m in c['handle remapping']['map'] if m['key']=='Data'),None)
gates=lambda c:c['bounds']['additionalInvalidatingHandles']
# SOURCE : 3 boutons natifs par voix, groupe de 3, sur v<n>_source à 3 valeurs
for v in range(8):
 for mode in range(3):
  c=next(c for c in page if c['componentData']['type']==f'source_select{v}_{mode}');assert param(c)==I[f'v{v}_source'] and c['bounds']['bounds']==f'{172+mode*152} 78 140 42'
  b=D[f'source_select{v}_{mode}']['componentsData'][0]['componentData']['data'];assert b['buttonId']==mode and b['numButtonsInGroup']==3
assert P[I['v0_source']]['options']==['SYNTH','WAVETABLE','SAMPLE'] and P[I['source']]['max']==2
# toutes les conditions sur SOURCE sont à 3 états
for c in page:
 for g in gates(c):
  m=re.fullmatch(r'IndexedEnabling/(\d+)/(\d+)/Parameter (\d+)',g)
  if m and P[int(m.group(3))]['key'].endswith('source'):assert m.group(2)=='3',g
# affichage : 8 tables x 8 voix, bande de 8 images 288 x 100 pilotée par POS (v<n>_wave)
views=[c for c in page if c['componentData']['type'].startswith('wt_view_')];assert len(views)==64
for c in views:
 v,t=map(int,c['componentData']['type'][8:].split('_'));assert param(c)==I[f'v{v}_wave']
 k=D[c['componentData']['type']]['componentsData'][0]['componentData']['data'];assert k['numFrames']==8 and k['filmStrip']==f'wt_{WT_TABLES[t].lower()}.png'
 assert f'IndexedEnabling/{t}/8/Parameter {I[f"v{v}_wt_table"]}' in gates(c) and f'IndexedEnabling/1/3/Parameter {I[f"v{v}_source"]}' in gates(c)
total=0
for t in WT_TABLES:
 im=Image.open(A/f'wt_{t.lower()}.png');assert im.size==(288,800);total+=im.size[0]*im.size[1]*4
assert total<=7.5e6,total
print(f'bandes WAVETABLE : {total/2**20:.1f} Mio décodées')
# listes TABLE et SCAN, ouvertes par les champs v<n>_wt_table__open / v<n>_wt_scan__open ; grille 4 x 2 de cases 140 x 42
for v in range(8):
 for k,x in [('wt_table',20),('wt_scan',324)]:assert any(param(c)==I[f'v{v}_{k}__open'] and c['bounds']['bounds']==f'{x} 132 140 42' for c in page) and any(param(c)==I[f'v{v}_{k}'] and c['bounds']['bounds']==f'{x+152} 132 140 42' for c in page)
assert any(c['componentData']['type']=='Image' and c['componentData']['data'].get('image')=='source_title.png' and c['bounds']['bounds']=='20 78 140 42' for c in page)
# Q-Links de TONE : les 7 d'avant à leur place, END et SCAN TIME ensuite (15, 11)
for fn in ['Q-Links.json','Q-Links - 8by1.json','Q-Links - 4by4.json']:
 m=json.loads((A/fn).read_text())['Screen Mode Q-Links']['map'][0]['Q-Links']
 assert [m[f'Q-Link {k}'] for k in [13,9,5,1,14,10,6,2,15]]==[I[f'tone_q{i}'] for i in range(9)]
dl=[c for c in page if c['componentData']['type'].startswith('dial')]
for v in range(8):
 for o,src in [(0,0),(0,2),(2,1)]:assert any(param(c)==I[f'tone_q{6+o}'] and f'IndexedEnabling/{src}/3/Parameter {I[f"v{v}_source"]}' in gates(c) for c in dl)
for v in range(8):
 for k in ['wt_table','wt_scan']:assert D[f'wt_value_{v}_{k}']['componentsData'][0]['componentData']['data']['textStyle']['justification']=='centred'
print('PASS skin WAVETABLE : SOURCE 3 boutons, conditions à 3 états, 64 vues de table sur POS, listes TABLE / SCAN, boutons TABLE / SCAN + valeur en texte, Q-Links TONE')
