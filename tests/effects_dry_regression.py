"""Compare exact int16 output to the validated pre-effects engine via its real API."""
import ctypes as C
from pathlib import Path
B=Path(__file__).resolve().parents[1]
Create=C.CFUNCTYPE(C.c_void_p,C.c_char_p);Destroy=C.CFUNCTYPE(None,C.c_void_p);Midi=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_uint8),C.c_int);Set=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_char_p);Get=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_char_p,C.c_char_p,C.c_int);Render=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int16),C.c_int)
class API(C.Structure):_fields_=[('create',Create),('destroy',Destroy),('midi',Midi),('set',Set),('get',Get),('render',Render)]
libs=[C.CDLL(str(p)) for p in [B/'Validation/effects/baseline-candidate6-x86_64.so',B/'Banks/PatternariumBest150/Compiled/pitonic-x86_64.so']]
eng=[]
for lib in libs:lib.mpc_engine.restype=C.POINTER(API);a=lib.mpc_engine().contents;eng.append((a,a.create(None)))
try:
 for block in range(1400):
  for api,ptr in eng:
   if block%23==0:
    note=36+(block//23)%8;api.midi(ptr,(C.c_uint8*3)(0x90,note,45+(block%83)),3)
   if block%100==0:
    api.set(ptr,b'v3_pan',str((block//100)*11-75).encode());api.set(ptr,b'v0_freq',str(40+block).encode())
  chunks=[]
  for api,ptr in eng:
   out=(C.c_int16*256)();api.render(ptr,out,128);chunks.append(bytes(out))
  assert chunks[0]==chunks[1],f'dry output changed in block {block}'
finally:
 for api,ptr in eng:api.destroy(ptr)
print('PASS: dry audio bit-identical to candidate 6 across 179200 frames, eight voices, retriggers and edits')
