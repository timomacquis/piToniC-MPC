"""Native XPL binary-state text encoding."""
ALPHABET='.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+'
def encode(data):
 result=[];carry=0;bits=0
 for byte in data:
  carry|=byte<<bits;bits+=8
  while bits>=6:result.append(ALPHABET[carry&63]);carry>>=6;bits-=6
 if bits:result.append(ALPHABET[carry&63])
 return str(len(data))+'.'+''.join(result)
def decode(text):
 n,data=text.split('.',1);out=bytearray();carry=0;bits=0
 for c in data:
  carry|=ALPHABET.index(c)<<bits;bits+=6
  while bits>=8:out.append(carry&255);carry>>=8;bits-=8
 return bytes(out[:int(n)])
