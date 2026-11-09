"""Exact old/new audio comparison for this display/Q-Link maintenance candidate."""
from pathlib import Path
import ctypes as C,struct,wave,tempfile,time
B=Path(__file__).resolve().parents[1]
Create=C.CFUNCTYPE(C.c_void_p,C.c_char_p);Destroy=C.CFUNCTYPE(None,C.c_void_p);Midi=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_uint8),C.c_int);Set=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p,C.c_char_p);Get=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_char_p,C.c_char_p,C.c_int);Render=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int16),C.c_int)
class API(C.Structure):_fields_=[('create',Create),('destroy',Destroy),('midi',Midi),('set',Set),('get',Get),('render',Render)]
paths=[B/'Validation/sample-1.0.1-candidate.2/baseline/pitonic-x86_64.so',B/'Banks/PatternariumBest150/Compiled/pitonic-x86_64.so']
libs=[C.CDLL(str(p)) for p in paths];instances=[]
for lib in libs:lib.mpc_engine.restype=C.POINTER(API);api=lib.mpc_engine().contents;instances.append((api,api.create(None)))
def value(api,p,key):
 buf=C.create_string_buffer(2048);assert api.get(p,key,buf,len(buf))>0;return buf.value
try:
 with tempfile.TemporaryDirectory(prefix='pitonic-audio-regression-') as directory:
  wavpath=Path(directory)/'comparison.wav'
  with wave.open(str(wavpath),'wb') as f:
   f.setnchannels(2);f.setsampwidth(2);f.setframerate(44100);f.writeframes(b''.join(struct.pack('<hh',(k*997)%20000-10000,(k*173)%16000-8000) for k in range(44100)))
  for mode in ['SYNTH','SAMPLE']:
   for api,p in instances:
    # Same deterministic INIT sound, no host/program-index dependency.
    api.set(p,b'v0_mix',b'0');api.set(p,b'v0_modamt',b'0');api.set(p,b'v0_drive',b'0');api.set(p,b'v0_eqgain',b'0')
    if mode=='SAMPLE':api.set(p,b'source',b'1');api.set(p,b'sample_path',str(wavpath).encode())
   if mode=='SAMPLE':
    for _ in range(1000):
     for api,p in instances:api.render(p,(C.c_int16*256)(),128)
     if all(value(api,p,b'sample_status')==b'Ready' for api,p in instances):break
     time.sleep(.002)
    else:raise AssertionError('Sample worker did not finish')
   for block in range(600):
    outputs=[]
    for api,p in instances:
     if block%37==0:api.midi(p,(C.c_uint8*3)(0x90,36,40+block%87),3)
     if block%100==0:
      api.set(p,b'tone_q0',str((block%500)/500).encode());api.set(p,b'tone_q1',str((block%300)/300).encode())
     out=(C.c_int16*256)();api.render(p,out,128);outputs.append(bytes(out))
    assert outputs[0]==outputs[1],(mode,block)
   print('PASS bit-identical old/new PCM:',mode,'600 blocks including contextual Q-Link changes')
finally:
 for api,p in instances:api.destroy(p)
