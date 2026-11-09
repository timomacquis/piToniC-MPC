/* πToniC WAV reader and bounded browser. Original implementation, MIT.
 * All filesystem access, allocation and disposal are worker-only. */
#pragma once
#include <pthread.h>
#include <stdatomic.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <strings.h>
#define SP_PATH 1024
#define SP_ENTRIES 256
#define SP_BINS 32
#define SP_MAX_FRAMES 1920000u
#define SP_MAX_BYTES (32u*1024u*1024u)
typedef struct {float *data;unsigned frames,rate;int channels;float peaks[SP_BINS],minima[SP_BINS],maxima[SP_BINS],preview_scale;char path[SP_PATH];} Sample;
/* Serpentine packing keeps adjacent filmstrip frames visually adjacent even
 * if a native host rounds a normalized value one frame differently. */
static unsigned sp_wave_code(const Sample *s,unsigned bin){unsigned negative=(unsigned)(-s->minima[bin]*s->preview_scale+.5f),positive=(unsigned)(s->maxima[bin]*s->preview_scale+.5f);if(negative>10)negative=10;if(positive>10)positive=10;return negative*11+((negative&1)?10-positive:positive);}
typedef struct {char name[256];int directory;} SPEntry;
typedef struct {
 pthread_t thread;pthread_mutex_t mutex;int started;atomic_int stop;
 char requested[8][SP_PATH];atomic_uint request[8];unsigned seen[8];
 _Atomic(Sample*) pending[8],retired[8];Sample *current[8];
 atomic_int status[8];atomic_uint revision;
 char directory[SP_PATH],navigation[SP_PATH];SPEntry entries[SP_ENTRIES];int count,page,overflow;
 unsigned nav_seen;atomic_uint nav_request;int browser,owner;
} SampleIO;
static unsigned sp_u16(const unsigned char *p){return p[0]|p[1]<<8;}
static unsigned sp_u32(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void sp_free(Sample *s){if(s){free(s->data);free(s);}}
static Sample *sp_read(const char *path,int *status){
 *status=3;FILE *f=fopen(path,"rb");if(!f){*status=4;return NULL;}
 struct stat st;if(fstat(fileno(f),&st)||!S_ISREG(st.st_mode)||st.st_size<44||st.st_size>SP_MAX_BYTES){fclose(f);return NULL;}
 unsigned char h[40];if(fread(h,1,12,f)!=12||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4)){fclose(f);return NULL;}
 unsigned fmt=0,ch=0,rate=0,bits=0,align=0,bytes=0;long data=0;
 while(ftell(f)+8<=st.st_size){if(fread(h,1,8,f)!=8)break;unsigned n=sp_u32(h+4);long pos=ftell(f);if((uint64_t)pos+n>(uint64_t)st.st_size)break;
 if(!memcmp(h,"fmt ",4)&&n>=16){unsigned m=n<40?n:40;if(fread(h,1,m,f)!=m)break;fmt=sp_u16(h);ch=sp_u16(h+2);rate=sp_u32(h+4);align=sp_u16(h+12);bits=sp_u16(h+14);
 if(fmt==65534&&n>=40&&m>=40){static const unsigned char tail[12]={0,0,16,0,128,0,0,170,0,56,155,113};if(memcmp(h+28,tail,12))fmt=0;else fmt=sp_u32(h+24);}}
 else if(!memcmp(h,"data",4)){data=pos;bytes=n;}
 if(fseek(f,pos+(long)n+(n&1),SEEK_SET))break;
 }
 if(!data||(fmt!=1&&fmt!=3)||(ch!=1&&ch!=2)||rate<8000||rate>192000||(bits!=16&&bits!=24&&bits!=32)||(fmt==3&&bits!=32)||align!=ch*(bits/8)||!bytes||bytes%align){fclose(f);return NULL;}
 unsigned frames=bytes/align;if(frames>rate*10u||frames>SP_MAX_FRAMES){*status=5;fclose(f);return NULL;}
 Sample *s=calloc(1,sizeof *s);if(!s){*status=6;fclose(f);return NULL;}s->data=malloc((size_t)frames*ch*sizeof(float));if(!s->data){sp_free(s);*status=6;fclose(f);return NULL;}
 s->frames=frames;s->rate=rate;s->channels=ch;snprintf(s->path,sizeof s->path,"%s",path);fseek(f,data,SEEK_SET);
 for(unsigned i=0;i<frames*ch;i++){if(fread(h,1,bits/8,f)!=bits/8){sp_free(s);fclose(f);return NULL;}float v;
 if(fmt==3){uint32_t u=sp_u32(h);memcpy(&v,&u,4);if(!isfinite(v)){sp_free(s);fclose(f);return NULL;}}
 else if(bits==16)v=(int16_t)sp_u16(h)/32768.f;
 else if(bits==24){int32_t v24=(int32_t)((unsigned)h[0]<<8|(unsigned)h[1]<<16|(unsigned)h[2]<<24);v=v24/2147483648.f;}
 else v=(int32_t)sp_u32(h)/2147483648.f;
 if(v>1)v=1;if(v< -1)v=-1;s->data[i]=v;unsigned bin=(unsigned)((uint64_t)(i/ch)*SP_BINS/frames);if(fabsf(v)>s->peaks[bin])s->peaks[bin]=fabsf(v);if(v<s->minima[bin])s->minima[bin]=v;if(v>s->maxima[bin])s->maxima[bin]=v;
 }
 float peak=0;for(unsigned bin=0;bin<SP_BINS;bin++)if(s->peaks[bin]>peak)peak=s->peaks[bin];s->preview_scale=peak>1e-12f?10.f/peak:0;
 fclose(f);*status=2;return s;
}
static int sp_entry_cmp(const void *a,const void *b){const SPEntry *x=a,*y=b;if(x->directory!=y->directory)return y->directory-x->directory;return strcasecmp(x->name,y->name);}
static void sp_scan(SampleIO *io,const char *path){SPEntry *list=calloc(SP_ENTRIES,sizeof *list);if(!list)return;int count=0,overflow=0;DIR *d=opendir(path);
 if(d){struct dirent *e;while((e=readdir(d))){if(e->d_name[0]=='.')continue;char full[SP_PATH];if(snprintf(full,sizeof full,"%s/%s",path,e->d_name)>=(int)sizeof full)continue;struct stat st;if(stat(full,&st))continue;int directory=S_ISDIR(st.st_mode);size_t len=strlen(e->d_name);if(!directory&&(!S_ISREG(st.st_mode)||len<4||strcasecmp(e->d_name+len-4,".wav")))continue;if(count==SP_ENTRIES){overflow=1;continue;}snprintf(list[count].name,256,"%s",e->d_name);list[count++].directory=directory;}closedir(d);qsort(list,count,sizeof *list,sp_entry_cmp);}
 pthread_mutex_lock(&io->mutex);memcpy(io->entries,list,SP_ENTRIES*sizeof *list);io->count=count;io->overflow=overflow;io->page=0;snprintf(io->directory,SP_PATH,"%s",path);pthread_mutex_unlock(&io->mutex);free(list);atomic_fetch_add(&io->revision,1);
}
static void *sp_worker(void *arg){SampleIO *io=arg;
 while(!atomic_load(&io->stop)){
 for(int i=0;i<8;i++){sp_free(atomic_exchange(&io->retired[i],NULL));unsigned seq=atomic_load(&io->request[i]);if(seq==io->seen[i])continue;char path[SP_PATH];pthread_mutex_lock(&io->mutex);snprintf(path,SP_PATH,"%s",io->requested[i]);pthread_mutex_unlock(&io->mutex);io->seen[i]=seq;int status=0;Sample *s=path[0]?sp_read(path,&status):calloc(1,sizeof(Sample));
 pthread_mutex_lock(&io->mutex);if(seq!=atomic_load(&io->request[i])){pthread_mutex_unlock(&io->mutex);sp_free(s);continue;}sp_free(atomic_exchange(&io->pending[i],s));atomic_store(&io->status[i],status);pthread_mutex_unlock(&io->mutex);atomic_fetch_add(&io->revision,1);
 }
 unsigned nav=atomic_load(&io->nav_request);if(nav!=io->nav_seen){char path[SP_PATH];pthread_mutex_lock(&io->mutex);snprintf(path,SP_PATH,"%s",io->navigation);pthread_mutex_unlock(&io->mutex);io->nav_seen=nav;sp_scan(io,path);}
 struct timespec delay={0,10000000};nanosleep(&delay,NULL);
 }return NULL;
}
static SampleIO *sp_create(void){SampleIO *io=calloc(1,sizeof *io);if(!io)return NULL;pthread_mutex_init(&io->mutex,NULL);strcpy(io->directory,"/media");return io;}
static void sp_destroy(SampleIO *io){if(!io)return;atomic_store(&io->stop,1);if(io->started)pthread_join(io->thread,NULL);for(int i=0;i<8;i++){sp_free(io->current[i]);sp_free(atomic_load(&io->pending[i]));sp_free(atomic_load(&io->retired[i]));}pthread_mutex_destroy(&io->mutex);free(io);}
static int sp_start(SampleIO *io){if(io->started)return 1;if(pthread_create(&io->thread,NULL,sp_worker,io))return 0;io->started=1;return 1;}
static int sp_request(SampleIO *io,int v,const char *path){if(!io||v<0||v>=8||strlen(path)>=SP_PATH||(path[0]&&path[0]!='/')||strchr(path,'\n')||strchr(path,'\r'))return 0;
 if(!sp_start(io)||pthread_mutex_trylock(&io->mutex))return 0;snprintf(io->requested[v],SP_PATH,"%s",path);atomic_fetch_add(&io->request[v],1);atomic_store(&io->status[v],path[0]?1:0);pthread_mutex_unlock(&io->mutex);return 1;}
static void sp_nav(SampleIO *io,const char *path){if(!io||strlen(path)>=SP_PATH||!sp_start(io)||pthread_mutex_trylock(&io->mutex))return;snprintf(io->navigation,SP_PATH,"%s",path);atomic_fetch_add(&io->nav_request,1);pthread_mutex_unlock(&io->mutex);}
/* Audio-side ownership transfer only; worker frees retired buffers. */
static int sp_commit(SampleIO *io,int i){if(!io||atomic_load(&io->retired[i]))return 0;Sample *s=atomic_exchange(&io->pending[i],NULL);if(!s)return 0;Sample *old=io->current[i];io->current[i]=s;atomic_store(&io->retired[i],old);return 1;}
