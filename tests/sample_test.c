#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include "../src/engine.c"
static void u16(FILE*f,unsigned x){fputc(x&255,f);fputc(x>>8&255,f);}
static void u32(FILE*f,unsigned x){u16(f,x&65535);u16(f,x>>16);}
static void wav(const char *path,int channels,int bits,int ieee,int rate,int frames){FILE*f=fopen(path,"wb");assert(f);unsigned bytes=frames*channels*bits/8;fwrite("RIFF",1,4,f);u32(f,36+bytes);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,ieee?3:1);u16(f,channels);u32(f,rate);u32(f,rate*channels*bits/8);u16(f,channels*bits/8);u16(f,bits);fwrite("data",1,4,f);u32(f,bytes);for(int i=0;i<frames;i++)for(int c=0;c<channels;c++){int32_t val=c?-1073741824:1073741824;if(ieee){float x=c?-.5f:.5f;uint32_t u;memcpy(&u,&x,4);u32(f,u);}else if(bits==16)u16(f,(uint32_t)val>>16);else if(bits==24){fputc(val>>8&255,f);fputc(val>>16&255,f);fputc(val>>24&255,f);}else u32(f,(uint32_t)val);}fclose(f);}
static void wait_status(PT*p,int status){int16_t block[256];for(int t=0;t<1000;t++){render(p,block,128);if(atomic_load(&p->samples->status[0])==status&&(status!=2||p->samples->current[0]))return;struct timespec ts={0,2000000};nanosleep(&ts,NULL);}fprintf(stderr,"Status %d != %d\n",atomic_load(&p->samples->status[0]),status);assert(0);}
static void hit(PT*p){uint8_t n[]={0x90,36,127};midi(p,n,3);}
static void neutral(PT*p){set(p,"source","1");set(p,"v0_mix","0");set(p,"v0_drive","0");set(p,"v0_eqgain","0");set(p,"v0_modamt","0");set(p,"v0_decay","10000");}
int main(void){char directory[]="/tmp/pitonic-sample-XXXXXX";assert(mkdtemp(directory));char path[SP_PATH];snprintf(path,sizeof path,"%s/Metal & wood ü.wav",directory);wav(path,2,24,0,48000,4800);
 PT*p=create(NULL),*q=create(NULL);assert(p->samples&&q->samples&&!p->samples->started&&!q->samples->started);neutral(p);set(p,"sample_path",path);wait_status(p,2);assert(p->samples->current[0]->channels==2&&p->samples->current[0]->rate==48000);assert(p->samples->current[0]->peaks[0]==.5f);
 hit(p);int16_t out[256];render(p,out,128);assert(out[64]>0&&out[65]<0);assert(fabs(p->position[0]-128*48000./44100)<.01);
 set(p,"sample_tune","12");hit(p);render(p,out,128);assert(fabs(p->position[0]-256*48000./44100)<.02);
 set(p,"sample_tune","-12");hit(p);render(p,out,128);assert(fabs(p->position[0]-64*48000./44100)<.02);
 set(p,"sample_start","50");hit(p);assert(p->position[0]==2400);render(p,out,128);
 set(p,"sample_tune","7");set(p,"compare","1");set(p,"sample_tune","-3");set(p,"sample_start","10");set(p,"compare","1");assert(p->tune[0][p->v[0].bank]==7);
 char saved[32768],check[32768];assert(get(p,"state",saved,sizeof saved)>0);assert(strstr(saved,"SM1 "));set(q,"state",saved);assert(q->state_valid);wait_status(q,2);assert(get(q,"state",check,sizeof check)>0&&!strcmp(saved,check));assert(!strcmp(q->sample_paths[0],path));
 char bad[32768];assert(strlen(saved)+8<sizeof bad);strcpy(bad,saved);strcat(bad," broken");set(q,"state",bad);assert(!q->state_valid);get(q,"state",check,sizeof check);assert(!strcmp(saved,check));
 set(p,"random","1");assert(p->source[0]==1&&!strcmp(p->sample_paths[0],path));set(p,"alter","1");assert(p->source[0]==1&&!strcmp(p->sample_paths[0],path));
 set(p,"tone_q0","0.5");assert(p->tune[0][p->v[0].bank]==0);set(p,"tone_q1","0.5");assert(fabs(p->start[0][p->v[0].bank]-49.95)<.01);assert(p->v[0].banks[p->v[0].bank][FREQ]>0);
 set(p,"v0_sample_tune","5");assert(p->tune[0][p->v[0].bank]==5);set(p,"v1_sample_start","23");assert(p->start[1][p->v[1].bank]==23);
 /* After worker changes settle, knob changes must never notify waveform bars. */
 struct timespec settle={0,30000000};nanosleep(&settle,NULL);char key[80];for(int k=0;k<1000&&mpc_engine_changed(p,key,sizeof key);k++);set(p,"tone_q0","0.5");int changed=0;while(mpc_engine_changed(p,key,sizeof key)){assert(strncmp(key,"sample_wave",11));assert(++changed<100);}
 /* Browser scan + directory navigation and exact original file path. */
 set(q,"browse_path_set",directory);for(int i=0;i<500;i++){int ready=0;pthread_mutex_lock(&q->samples->mutex);ready=!strcmp(q->samples->directory,directory)&&q->samples->count==1;pthread_mutex_unlock(&q->samples->mutex);if(ready)break;struct timespec ts={0,2000000};nanosleep(&ts,NULL);}
 set(q,"load","1");set(q,"browse_item0","1");wait_status(q,2);assert(!strcmp(q->sample_paths[0],path));
 for(int fmt=0;fmt<4;fmt++){char f[SP_PATH];snprintf(f,sizeof f,"%s/f%d.wav",directory,fmt);wav(f,1,fmt==0?16:fmt==1?24:32,fmt==3,44100,1000);int st;Sample *sm=sp_read(f,&st);assert(sm&&st==2&&fabs(sm->data[0]-.5f)<1e-6);sp_free(sm);unlink(f);}
 char longfile[SP_PATH];snprintf(longfile,sizeof longfile,"%s/long.wav",directory);wav(longfile,1,16,0,8000,80001);int st;assert(!sp_read(longfile,&st)&&st==5);unlink(longfile);
 set(q,"sample_path","/missing-pitonic-file.wav");wait_status(q,4);char error_name[256];assert(get(q,"sample_name_display",error_name,sizeof error_name)>0&&strstr(error_name,"Missing / unreadable file"));hit(q);set(q,"v0_mix","0");render(q,out,128);for(int i=0;i<256;i++)assert(out[i]==0);
 /* Legacy state clears sample configuration and stays readable. */
 PT*legacy=create(NULL);char old[32768];get(legacy,"state",old,sizeof old);assert(!strstr(old,"SM1"));set(q,"state",old);assert(q->state_valid&&!q->source[0]&&!q->sample_paths[0][0]);destroy(legacy);
 for(int i=0;i<50;i++){set(p,"sample_path",path);int16_t tiny[2];render(p,tiny,1);}destroy(p);destroy(q);unlink(path);rmdir(directory);
 puts("PASS SAMPLE: PCM16/24/32 + float32, stereo, sample rates, octave tuning, START, A/B, paths and state roundtrip, malformed rollback, missing/long files, browser, random protection and worker teardown");
}
