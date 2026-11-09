#define main previous_sample_test_main
#include "sample_test.c"
#undef main
int main(void){Sample s={0};s.preview_scale=10;for(int negative=0;negative<=10;negative++)for(int positive=0;positive<=10;positive++){
 s.minima[0]=-negative/10.f;s.maxima[0]=positive/10.f;unsigned code=sp_wave_code(&s,0);assert(code<=120);int row=code/11,slot=code%11,pos=row&1?10-slot:slot;assert(row==negative&&pos==positive);}
 for(int code=0;code<120;code++){int n=code/11,k=code%11,p=n&1?10-k:k;int next=code+1,nn=next/11,kk=next%11,pp=nn&1?10-kk:kk;assert(abs(nn-n)+abs(pp-p)==1);}
 s.preview_scale=10000;s.minima[0]=-.001f;s.maxima[0]=.0005f;assert(sp_wave_code(&s,0)==115);
 char path[]="/tmp/pitonic-preview-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);FILE*f=fopen(path,"wb");assert(f);int frames=3200;fwrite("RIFF",1,4,f);u32(f,36+frames*2);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,1);u32(f,32000);u32(f,64000);u16(f,2);u16(f,16);fwrite("data",1,4,f);u32(f,frames*2);for(int i=0;i<frames;i++)u16(f,(i/100)&1?(uint16_t)-16384:8192);fclose(f);
 int status;Sample*read=sp_read(path,&status);assert(read&&status==2);for(int i=0;i<32;i++){if(i&1){assert(read->minima[i]==-.5f&&read->maxima[i]==0&&read->peaks[i]==.5f);}else assert(read->minima[i]==0&&read->maxima[i]==.25f&&read->peaks[i]==.25f);}
 PT*e=create(NULL);e->samples->current[0]=read;atomic_store(&e->samples->status[0],2);char b[32];for(int i=0;i<32;i++){char key[32];snprintf(key,sizeof key,"sample_wave%d",i);assert(get(e,key,b,sizeof b)>0);int code=(int)roundf(strtof(b,0)*127);assert(code==(int)sp_wave_code(read,i));}
 e->samples->current[0]=NULL;sp_free(read);destroy(e);unlink(path);puts("PASS actual PCM extrema: positive-only and negative-only bins preserved, all 121 bipolar states round-trip through the normalized handle, adjacent frames remain visually adjacent; no added parameter indices");}
