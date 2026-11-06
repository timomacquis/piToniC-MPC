/* πToniC. Copyright 2026 Timo. MIT.
 * Voice architecture and basic wave/noise/envelope building blocks adapted from
 * Weird Dreams (Vincent Fillion), based on WeirdDrums (Daniele Filaretti, MIT).
 * No Microtonic binaries, presets, algorithms or TR GPL source incorporated.
 * Native MPC engine: shared send effects; no file I/O, allocations or locks in render/set/get.
 */
#include "engine.h"
#include "fields.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#define SR 44100.f
#define PI 3.14159265358979323846f
#define TABLE 4096
#include "effects.h"
enum {FREQ,WAVE,ATK,DCY,MODE,AMT,RATE,MVEL,MIX,FILTER,CUT,RES,NATK,NDCY,ENV,STEREO,DRIVE,EQF,EQG,LEVEL,OVEL,NVEL,PAN,CHOKE};
typedef struct {float value,attack_coeff,decay_coeff;int age,attack,decay,linear;} Env;
typedef struct {float b0,b1,b2,a1,a2,z1,z2;} Biquad;
typedef struct {
 float banks[2][PT_FIELDS];int bank,mute,solo;
 Env amp,noise,pitch; Biquad filter[2],eq[2];
 float phase,lfo,random_lp,random_held,random_a,mod_vel,osc_vel,noise_vel;
 float pan_l,pan_r,mix_osc,mix_noise,drive,level,level_target;
 float last[2],fade[2];int cross,release,active,clap_left,clap_timer,clap_gap;
 uint32_t noise_seed[2],mod_seed;uint64_t started;
} Voice;
typedef struct {
 Voice v[8];int selected,state_valid;uint64_t clock;
 uint8_t changed[8*(PT_FIELDS+2)+1+4+8+8+24];unsigned cursor;
 float sine[TABLE+1],exp2tab[TABLE+1],tanh_table[TABLE+1];
 uint32_t rng;
 uint64_t activity_until[8];uint8_t activity[8],activity_dirty[8];
 Effects fx;float fx_time_accum;float sends[8][2],send_smooth[8][2];uint8_t fx_dirty[29];
} PT;
static float clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static float lookup(const float *t,float n){float p=clamp(n,0,1)*TABLE;int i=(int)p;if(i==TABLE)return t[i];return t[i]+(t[i+1]-t[i])*(p-i);}
static float sine(PT *p,float phase){phase-=floorf(phase);return lookup(p->sine,phase);}
static float ratio(PT *p,float semi){return lookup(p->exp2tab,(clamp(semi,-96,96)+96)/192);}
static float saturate(PT *p,float x){return lookup(p->tanh_table,(clamp(x,-8,8)+8)/16);}
static float random_sample(uint32_t *s){uint32_t x=*s;x^=x<<13;x^=x>>17;x^=x<<5;*s=x;return (float)(x>>8)*(2.f/16777216.f)-1;}
static void env_start(Env *e,float attack,float decay,int linear){
 e->age=0;e->attack=(int)(attack*.001f*SR);e->decay=(int)fmaxf(1,decay*.001f*SR);e->linear=linear;
 e->value=e->attack?0.0001f:1.f;e->attack_coeff=e->attack?expf(logf(10000.f)/e->attack):1;
 e->decay_coeff=expf(logf(.0001f)/e->decay);
}
static float env_next(Env *e){
 if(e->age<e->attack){e->value=e->linear?(float)(e->age+1)/e->attack:e->value*e->attack_coeff;}
 else if(e->age<e->attack+e->decay){e->value=e->linear?1.f-(float)(e->age-e->attack)/e->decay:e->value*e->decay_coeff;}
 else e->value=0;
 if(e->age<INT_MAX)e->age++;return e->value;
}
static void biquad_set(Biquad *b,float hz,float q,int type,float gain){
 float w=2*PI*clamp(hz,20,SR*.45f)/SR,c=cosf(w),a=sinf(w)/(2*clamp(q,.1f,20)),a0;
 if(type==3){float A=powf(10,gain/40);a0=1+a/A;b->b0=(1+a*A)/a0;b->b1=-2*c/a0;b->b2=(1-a*A)/a0;b->a1=b->b1;b->a2=(1-a/A)/a0;}
 else{a0=1+a;if(type==0){b->b0=(1-c)*.5f/a0;b->b1=(1-c)/a0;b->b2=b->b0;}else if(type==2){b->b0=(1+c)*.5f/a0;b->b1=-(1+c)/a0;b->b2=b->b0;}else{b->b0=a/a0;b->b1=0;b->b2=-a/a0;}b->a1=-2*c/a0;b->a2=(1-a)/a0;}
}
static float biquad(Biquad *b,float x){float y=b->b0*x+b->z1;b->z1=b->b1*x-b->a1*y+b->z2;b->z2=b->b2*x-b->a2*y;return y;}
static float *values(Voice *v){return v->banks[v->bank];}
static void cache(Voice *v){float *f=values(v);float angle=(f[PAN]/100+1)*PI*.25f;
 v->pan_l=cosf(angle);v->pan_r=sinf(angle);v->mix_osc=sqrtf(1-f[MIX]/100);v->mix_noise=sqrtf(f[MIX]/100);
 v->drive=powf(10,f[DRIVE]/20);v->level_target=f[LEVEL]/100;
 v->random_a=1-expf(-2*PI*clamp(f[RATE],0,20000)/SR);
 for(int k=0;k<2;k++){biquad_set(&v->filter[k],f[CUT],f[RES],(int)f[FILTER],0);biquad_set(&v->eq[k],f[EQF],.707f,3,f[EQG]);}
}
static void notify_voice(PT *p,int v){for(int i=0;i<PT_FIELDS+2;i++)p->changed[1+v*(PT_FIELDS+2)+i]=1;if(v==p->selected)for(int i=0;i<24;i++)p->changed[1+8*(PT_FIELDS+2)+4+8+8+i]=1;p->changed[1+8*(PT_FIELDS+2)+4+8+v]=1;}
static void release(Voice *v){if(v->active)v->release=48;}
static float velocity(float val,float sensitivity){return clamp(1-(1-val)*sensitivity/100,0,1);}
static void trigger(PT *p,int index,int vel){Voice *v=&p->v[index];float *f=values(v);int solo=0;for(int i=0;i<8;i++)solo|=p->v[i].solo;
 if(v->mute||(solo&&!v->solo))return;
 int group=(int)f[CHOKE];if(group)for(int i=0;i<8;i++)if(i!=index&&(int)values(&p->v[i])[CHOKE]==group){if(i<index&&p->v[i].active&&p->v[i].started==p->clock)return;release(&p->v[i]);}
 p->activity_until[index]=p->clock+6174;if(!p->activity[index]){p->activity[index]=1;p->activity_dirty[index]=1;}
 v->fade[0]=v->last[0];v->fade[1]=v->last[1];v->cross=32;v->active=1;v->release=0;v->started=p->clock;v->phase=0;v->lfo=0;
 float norm=vel/127.f;v->osc_vel=velocity(norm,f[OVEL]);v->noise_vel=velocity(norm,f[NVEL]);v->mod_vel=velocity(norm,f[MVEL]);
 v->random_held=random_sample(&v->mod_seed);v->random_lp=v->random_held;
 env_start(&v->amp,f[ATK],f[DCY],0);env_start(&v->noise,f[NATK],f[NDCY],(int)f[ENV]==1);
 env_start(&v->pitch,0,fmaxf(1,f[RATE]),0);
 v->clap_left=(int)f[ENV]==2?3:0;v->clap_gap=(int)(SR*.01f);v->clap_timer=v->clap_gap;
 for(int k=0;k<2;k++){v->filter[k].z1=v->filter[k].z2=0;v->eq[k].z1=v->eq[k].z2=0;}
}
static float blep(float t,float dt){if(t<dt){t/=dt;return t+t-t*t-1;}if(t>1-dt){t=(t-1)/dt;return t*t+t+t+1;}return 0;}
static float osc(PT *p,Voice *v,float hz,float morph){
 float t=v->phase,dt=clamp(hz/SR,.00001f,.45f),w=morph*.03f,result;
 if(w<1){float sn=sine(p,t);result=w==0?sn:sn+(1-4*fabsf(t-.5f)-sn)*w;}
 else if(w<2){float tri=1-4*fabsf(t-.5f),saw=2*t-1-blep(t,dt);result=tri+(saw-tri)*(w-1);}
 else{float saw=2*t-1-blep(t,dt),sq=(t<.5f?1:-1)+blep(t,dt)-blep(fmodf(t+.5f,1),dt);result=saw+(sq-saw)*(w-2);}
 v->phase+=dt;if(v->phase>=1)v->phase-=1;return result;
}
static void render(void *ptr,int16_t *out,int frames){PT *p=ptr;int usefx=p->fx.rev_left||p->fx.del_left;
 for(int i=0;i<8;i++)if(p->v[i].active&&(p->sends[i][0]>0||p->sends[i][1]>0||p->send_smooth[i][0]>1e-7f||p->send_smooth[i][1]>1e-7f))usefx=1;
 for(int n=0;n<frames;n++){
 float sum[2]={0,0},rev[2]={0,0},del[2]={0,0};for(int i=0;i<8;i++){
 if(usefx)for(int k=0;k<2;k++){p->send_smooth[i][k]+=.002f*(p->sends[i][k]/100-p->send_smooth[i][k]);if(p->send_smooth[i][k]<1e-7f)p->send_smooth[i][k]=0;}Voice *v=&p->v[i];if(!v->active)continue;float *f=values(v),mod;
 if((int)f[MODE]==0)mod=env_next(&v->pitch)*f[AMT];else if((int)f[MODE]==1){mod=sine(p,v->lfo)*clamp(f[AMT],-48,48);v->lfo+=clamp(f[RATE],0,2000)/SR;v->lfo-=floorf(v->lfo);}else{if(f[RATE]>0)v->random_lp+=v->random_a*(random_sample(&v->mod_seed)-v->random_lp);mod=v->random_lp*clamp(f[AMT],-48,48);}
 float tone=osc(p,v,clamp(f[FREQ]*ratio(p,mod*v->mod_vel),20,SR*.45f),f[WAVE])*env_next(&v->amp)*v->osc_vel*v->mix_osc;
 if(v->clap_left&&--v->clap_timer<=0){v->clap_left--;v->clap_timer=v->clap_gap;env_start(&v->noise,0,f[NDCY],0);}
 float noise=env_next(&v->noise)*v->noise_vel*v->mix_noise;
 float nl=0,nr=0;if(v->mix_noise>0){nl=biquad(&v->filter[0],random_sample(&v->noise_seed[0]));nr=f[STEREO]>.5f?biquad(&v->filter[1],random_sample(&v->noise_seed[1])):nl;}
 v->level+=.002f*(v->level_target-v->level);
 for(int k=0;k<2;k++){float x=tone+(k?nr:nl)*noise;if(f[DRIVE]>.01f)x=saturate(p,x*v->drive);if(fabsf(f[EQG])>.001f)x=biquad(&v->eq[k],x);
 x*=v->level*(k?v->pan_r:v->pan_l);if(v->cross)x=x*(1-v->cross/32.f)+v->fade[k]*(v->cross/32.f);if(v->release)x*=v->release/48.f;v->last[k]=x;sum[k]+=x;if(usefx){rev[k]+=x*p->send_smooth[i][0];del[k]+=x*p->send_smooth[i][1];}}
 if(v->cross)v->cross--;if(v->release&&!--v->release)v->active=0;
 if((v->amp.value==0||v->mix_osc==0)&&(v->noise.value==0||v->mix_noise==0)&&(!v->clap_left||v->mix_noise==0)&& !v->cross)v->active=0;
 }float wet[2]={0,0};if(usefx)fx_process(&p->fx,rev,del,wet);for(int k=0;k<2;k++)out[n*2+k]=(int16_t)(clamp((sum[k]+wet[k])*.25f,-1,1)*32767);p->clock++;
}for(int i=0;i<8;i++)if(p->activity[i]&&p->clock>=p->activity_until[i]){p->activity[i]=0;p->activity_dirty[i]=1;}
}
static void *create(const char *dir){(void)dir;PT *p=calloc(1,sizeof *p);if(!p)return NULL;p->state_valid=1;p->rng=0x70695443;fx_init(&p->fx);
 for(int i=0;i<=TABLE;i++){float t=(float)i/TABLE;p->sine[i]=sinf(2*PI*t);p->exp2tab[i]=exp2f(-8+16*t);p->tanh_table[i]=tanhf(-8+16*t);}
 for(int i=0;i<8;i++){Voice *v=&p->v[i];for(int j=0;j<PT_FIELDS;j++)v->banks[0][j]=v->banks[1][j]=pt_fields[j].def;v->noise_seed[0]=123456789+i*977;v->noise_seed[1]=987654321+i*991;v->mod_seed=1009+i*997;cache(v);v->level=v->level_target;}
 return p;
}
static void destroy(void *p){free(p);}
static int parse_key(const char *k,int *v,int *f){int n=0;if(sscanf(k,"v%d_%n",v,&n)!=1||*v<0||*v>7||n<3)return 0;for(int j=0;j<PT_FIELDS;j++)if(!strcmp(k+n,pt_fields[j].key)){*f=j;return 1;}if(!strcmp(k+n,"mute")){*f=PT_FIELDS;return 1;}if(!strcmp(k+n,"solo")){*f=PT_FIELDS+1;return 1;}return 0;}
static int alias(const char *k,int *field){int page,i,n=0;if(sscanf(k,"q%d_%d%n",&page,&i,&n)==2&&k[n]==0&&page>=0&&page<3&&i>=0&&i<8){*field=page*8+i;return 1;}return 0;}
static void change(PT *p,int v,int f,float x){Voice *voice=&p->v[v];float old=f==PT_FIELDS?voice->mute:f==PT_FIELDS+1?voice->solo:values(voice)[f];if(f>=PT_FIELDS)x=x>.5f;else{x=clamp(x,pt_fields[f].lo,pt_fields[f].hi);if(pt_fields[f].options)x=roundf(x);}if(old==x)return;if(f>=PT_FIELDS){int on=x>.5f;if(f==PT_FIELDS)voice->mute=on;else voice->solo=on;int solo=0;for(int i=0;i<8;i++)solo|=p->v[i].solo;for(int i=0;i<8;i++)if(p->v[i].mute||(solo&&!p->v[i].solo))release(&p->v[i]);}else{const PTField *d=&pt_fields[f];x=clamp(x,d->lo,d->hi);if(d->options)x=roundf(x);values(voice)[f]=x;cache(voice);}p->changed[1+v*(PT_FIELDS+2)+f]=1;if(v==p->selected&&f<PT_FIELDS)p->changed[1+8*(PT_FIELDS+2)+4+8+8+f]=1;}
static int protected(int f){return f==LEVEL||f==PAN||f==CHOKE;}
static void randomize(PT *p,int small){Voice *v=&p->v[p->selected];if(!v->bank){memcpy(v->banks[1],v->banks[0],sizeof v->banks[0]);v->bank=1;}
 float *f=values(v);for(int j=0;j<PT_FIELDS;j++){if(protected(j))continue;const PTField *d=&pt_fields[j];float u=(random_sample(&p->rng)+1)*.5f;
 float lo=d->lo,hi=d->hi;if(j==FREQ)hi=10000;if(j==ATK||j==NATK)hi=80;if(j==DCY||j==NDCY)hi=1800;if(j==DRIVE)hi=20;if(j==EQG){lo=-9;hi=9;}if(j==RES)hi=5;
 float x=small?f[j]+(u-.5f)*(hi-lo)*.08f:lo+u*(hi-lo);if(!small&&(j==FREQ||j==CUT||j==EQF||j==DCY||j==NDCY))x=fmaxf(lo,1)*powf(hi/fmaxf(lo,1),u);if(d->options)x=small?f[j]:floorf(u*d->options);f[j]=clamp(d->options?roundf(x):x,d->lo,d->hi);}
 cache(v);notify_voice(p,p->selected);
}
static void state_set(PT *p,const char *str){Voice staged[8];memcpy(staged,p->v,sizeof staged);const char *s=str;int n=0,selected;
 p->state_valid=0;if(sscanf(s,"PT1 %d %n",&selected,&n)!=1||!n||selected<0||selected>7)return;s+=n;
 for(int v=0;v<8;v++){int bank,mute,solo;n=0;if(sscanf(s,"%d %d %d %n",&bank,&mute,&solo,&n)!=3||!n||bank<0||bank>1||mute<0||mute>1||solo<0||solo>1)return;s+=n;staged[v].bank=bank;staged[v].mute=mute;staged[v].solo=solo;
 for(int b=0;b<2;b++)for(int f=0;f<PT_FIELDS;f++){char *end;errno=0;float x=strtof(s,&end);const PTField *d=&pt_fields[f];if(end==s||errno||!isfinite(x)||x<d->lo||x>d->hi||(d->options&&x!=roundf(x)))return;staged[v].banks[b][f]=x;s=end;}}
 float send[8][2]={{0}},fx[FX_COUNT];memcpy(fx,fx_defaults,sizeof fx);
 while(*s==' '||*s=='\n'||*s=='\r'||*s=='\t')s++;
 if(*s){if(strncmp(s,"FX1 ",4))return;s+=4;
 for(int i=0;i<28;i++){char *end;errno=0;float x=strtof(s,&end);float lo=i<16?0:fx_lo[i-16],hi=i<16?100:fx_hi[i-16];
 if(end==s||errno||!isfinite(x)||x<lo||x>hi||(i>=16&&(i-16==REV_ON||i-16==DEL_ON||i-16==DEL_SYNC||i-16==DEL_DIV)&&x!=roundf(x)))return;
 if(i<16)send[i/2][i%2]=x;else fx[i-16]=x;s=end;}
 while(*s==' '||*s=='\n'||*s=='\r'||*s=='\t')s++;if(*s)return;}
 memcpy(p->sends,send,sizeof send);memset(p->send_smooth,0,sizeof p->send_smooth);
 float bpm=p->fx.bpm;if(p->fx.rev_left||p->fx.del_left)fx_init(&p->fx);p->fx.bpm=bpm;memcpy(p->fx.param,fx,sizeof fx);p->fx.ready=0;p->fx_time_accum=0;fx_cache(&p->fx);memset(p->fx_dirty,1,sizeof p->fx_dirty);
 for(int v=0;v<8;v++){memcpy(p->v[v].banks,staged[v].banks,sizeof staged[v].banks);p->v[v].bank=staged[v].bank;p->v[v].mute=staged[v].mute;p->v[v].solo=staged[v].solo;p->v[v].active=0;if(p->activity[v]){p->activity[v]=0;p->activity_dirty[v]=1;}cache(&p->v[v]);notify_voice(p,v);}p->selected=selected;p->changed[0]=1;p->state_valid=1;
}
static void set(void *ptr,const char *key,const char *val){PT *p=ptr;if(!strcmp(key,"state")){state_set(p,val);return;}char *end;float x=strtof(val,&end);if(end==val||*end||!isfinite(x))return;
 if(!strcmp(key,"fx_time")){int fi=p->fx.param[DEL_SYNC]>.5f?DEL_DIV:DEL_MS;float value;
 if(fi==DEL_DIV){float delta=clamp(x,0,1)-p->fx.param[DEL_DIV]/4;
 if(fabsf(delta)<.125f){p->fx_time_accum+=delta;value=p->fx.param[DEL_DIV];if(fabsf(p->fx_time_accum)>=.04f){value=clamp(value+(p->fx_time_accum>0?1:-1),0,4);p->fx_time_accum=0;}}
 else{value=roundf(clamp(x,0,1)*4);p->fx_time_accum=0;}}
 else value=20+1480*clamp(x,0,1);if(value!=p->fx.param[fi]){p->fx.param[fi]=value;fx_cache(&p->fx);p->fx_dirty[16+fi]=p->fx_dirty[28]=1;}return;}
 if(!strcmp(key,"lfo_bpm")){if(x>=20&&x<=400){p->fx.bpm=x;fx_cache(&p->fx);}return;}
 int si,sn=0;char which[16];if(!strncmp(key,"send",4)&&sscanf(key,"send%d_%15s%n",&si,which,&sn)==2&&key[sn]==0&&si>=0&&si<8){int k=!strcmp(which,"reverb")?0:!strcmp(which,"delay")?1:-1;if(k>=0){x=clamp(x,0,100);if(p->sends[si][k]!=x){p->sends[si][k]=x;p->fx_dirty[si*2+k]=1;}}return;}
 if(key[0]=='r'||key[0]=='d')for(int i=0;i<FX_COUNT;i++)if(!strcmp(key,fx_keys[i])){x=clamp(x,fx_lo[i],fx_hi[i]);if(i==REV_ON||i==DEL_ON||i==DEL_SYNC||i==DEL_DIV)x=roundf(x);if(p->fx.param[i]!=x){p->fx.param[i]=x;if(i==DEL_SYNC||i==DEL_DIV)p->fx_time_accum=0;fx_cache(&p->fx);p->fx_dirty[16+i]=1;if(i==DEL_DIV||i==DEL_MS||i==DEL_SYNC)p->fx_dirty[28]=1;}return;}
 if(!strcmp(key,"selected")){p->selected=(int)clamp(roundf(x),0,7);p->changed[0]=1;notify_voice(p,p->selected);return;}
 int v,f,n=0;if(sscanf(key,"select%d%n",&v,&n)==1&&key[n]==0&&v>=0&&v<8){if(x>.5f){p->selected=v;p->changed[0]=1;notify_voice(p,v);}return;}
 if(parse_key(key,&v,&f)){change(p,v,f,x);return;}if(alias(key,&f)){change(p,p->selected,f,x);return;}
 if(x<=.5f)return;if(!strcmp(key,"trigger"))trigger(p,p->selected,110);
 else if(!strcmp(key,"alter"))randomize(p,1);else if(!strcmp(key,"random"))randomize(p,0);
 else if(!strcmp(key,"compare")){Voice *voice=&p->v[p->selected];float keep[3]={values(voice)[LEVEL],values(voice)[PAN],values(voice)[CHOKE]};voice->bank^=1;values(voice)[LEVEL]=keep[0];values(voice)[PAN]=keep[1];values(voice)[CHOKE]=keep[2];cache(voice);notify_voice(p,p->selected);}
}
static int get(void *ptr,const char *key,char *buf,int len){PT *p=ptr;int v,f,n=0;float x;
 if(len<2)return 0;if(!strcmp(key,"state_valid"))return snprintf(buf,len,"%d",p->state_valid);
 if(!strcmp(key,"state")){int used=snprintf(buf,len,"PT1 %d ",p->selected);for(int i=0;i<8;i++){Voice *v=&p->v[i];if(used>=len)return 0;used+=snprintf(buf+used,len-used,"%d %d %d ",v->bank,v->mute,v->solo);for(int b=0;b<2;b++)for(int f=0;f<PT_FIELDS;f++){if(used>=len)return 0;used+=snprintf(buf+used,len-used,"%.9g ",v->banks[b][f]);}}int extended=memcmp(p->fx.param,fx_defaults,sizeof fx_defaults)!=0;for(int v=0;v<8;v++)extended|=p->sends[v][0]!=0||p->sends[v][1]!=0;
 if(extended){if(used>=len)return 0;used+=snprintf(buf+used,len-used,"FX1 ");for(int i=0;i<28;i++){if(used>=len)return 0;float x=i<16?p->sends[i/2][i%2]:p->fx.param[i-16];used+=snprintf(buf+used,len-used,"%.9g ",x);}}
 return used<len?used:0;}
 if(sscanf(key,"activity%d%n",&v,&n)==1&&key[n]==0&&v>=0&&v<8)return snprintf(buf,len,"%d",p->activity[v]);
 if(!strcmp(key,"selected"))return snprintf(buf,len,"%d",p->selected);
 if(sscanf(key,"ab%d%n",&v,&n)==1&&key[n]==0&&v>=0&&v<8)return snprintf(buf,len,"%d",p->v[v].bank);
 if(!strcmp(key,"fx_time"))return snprintf(buf,len,"%.9g",p->fx.param[DEL_SYNC]>.5f?p->fx.param[DEL_DIV]/4:(p->fx.param[DEL_MS]-20)/1480);
 if(!strcmp(key,"fx_time_display"))return get(ptr,p->fx.param[DEL_SYNC]>.5f?"del_div_display":"del_ms_display",buf,len);
 int si,sn=0;char which[16];if(!strncmp(key,"send",4)&&sscanf(key,"send%d_%15s%n",&si,which,&sn)==2&&key[sn]==0&&si>=0&&si<8){int display=strstr(which,"_display")!=NULL;if(display)*strstr(which,"_display")=0;int k=!strcmp(which,"reverb")?0:!strcmp(which,"delay")?1:-1;if(k>=0)return snprintf(buf,len,display?"%.0f %%":"%.9g",p->sends[si][k]);}
 if(key[0]=='r'||key[0]=='d')for(int i=0;i<FX_COUNT;i++){size_t length=strlen(fx_keys[i]);int match=!strncmp(key,fx_keys[i],length);int display=match&&!strcmp(key+length,"_display");if((match&&key[length]==0)||display){float x=p->fx.param[i];if(display){if(i==REV_ON||i==DEL_ON)return snprintf(buf,len,"%s",x>.5f?"ON":"OFF");if(i==DEL_SYNC)return snprintf(buf,len,"%s",x>.5f?"SYNC":"FREE");if(i==DEL_DIV){static const char *div[]={"1/16","1/8","1/8 D","1/4","1/2"};return snprintf(buf,len,"%s",div[(int)x]);}return snprintf(buf,len,i==REV_DECAY?"%.1f s":i==DEL_MS?"%.0f ms":"%.0f %%",x);}return snprintf(buf,len,"%.9g",x);}}
 char bare[80];size_t k=strlen(key);int display=k>8&&!strcmp(key+k-8,"_display"),name=k>5&&!strcmp(key+k-5,"_name");if(k>=sizeof bare)return 0;strcpy(bare,key);if(display)bare[k-8]=0;if(name)bare[k-5]=0;
 if(alias(bare,&f))v=p->selected;else if(!parse_key(bare,&v,&f)){if(!strncmp(key,"select",6)||!strcmp(key,"trigger")||!strcmp(key,"alter")||!strcmp(key,"random")||!strcmp(key,"compare"))return snprintf(buf,len,"0");return 0;}
 x=f==PT_FIELDS?p->v[v].mute:f==PT_FIELDS+1?p->v[v].solo:values(&p->v[v])[f];
 if(name&&f<PT_FIELDS)return snprintf(buf,len,"%s",pt_fields[f].name);
 if(display&&f<PT_FIELDS){
 static const char *opts[PT_FIELDS][4]={[MODE]={"DECAY","SINE","RANDOM"},[FILTER]={"LP","BP","HP"},[ENV]={"EXP","LINEAR","CLAP"},[STEREO]={"OFF","ON"},[CHOKE]={"OFF","GROUP 1","GROUP 2","GROUP 3"}};
 if(pt_fields[f].options)return snprintf(buf,len,"%s",opts[f][(int)x]);
 const char *u=pt_fields[f].unit;if(f==RATE)u=(int)values(&p->v[v])[MODE]==0?"ms":"Hz";if(f==PAN)return snprintf(buf,len,"%.0f %s",fabsf(x),x==0?"C":x<0?"L":"R");return snprintf(buf,len,"%.2f %s",x,u);}
 return snprintf(buf,len,"%.9g",x);
}
static void midi(void *ptr,const uint8_t *m,int len){PT *p=ptr;if(len<3)return;int type=m[0]&0xf0;if(type==0xb0&&(m[1]==120||m[1]==123)){for(int i=0;i<8;i++)release(&p->v[i]);return;}if(type!=0x90||!m[2]||m[2]>127)return;int note=m[1],v=note<8?note:note>=36&&note<=43?note-36:-1;if(v>=0)trigger(p,v,m[2]);}
int mpc_engine_changed(void *ptr,char *key,int len){PT *p=ptr;for(int i=0;i<8;i++)if(p->activity_dirty[i]){p->activity_dirty[i]=0;snprintf(key,len,"activity%d",i);return 1;}for(int i=0;i<29;i++)if(p->fx_dirty[i]){p->fx_dirty[i]=0;if(i<16)snprintf(key,len,"send%d_%s",i/2,i%2?"delay":"reverb");else if(i<28)snprintf(key,len,"%s",fx_keys[i-16]);else snprintf(key,len,"fx_time");return 1;}unsigned total=sizeof p->changed;for(unsigned i=0;i<total;i++){unsigned at=p->cursor++%total;if(!p->changed[at])continue;p->changed[at]=0;if(at==0)snprintf(key,len,"selected");else if(at<1+8*(PT_FIELDS+2)){unsigned a=at-1,f=a%(PT_FIELDS+2);snprintf(key,len,"v%u_%s",a/(PT_FIELDS+2),f<PT_FIELDS?pt_fields[f].key:f==PT_FIELDS?"mute":"solo");}else if(at>=1+8*(PT_FIELDS+2)+4+8+8){unsigned a=at-(1+8*(PT_FIELDS+2)+4+8+8);snprintf(key,len,"q%u_%u",a/8,a%8);}else if(at>=1+8*(PT_FIELDS+2)+4+8)snprintf(key,len,"ab%u",at-(1+8*(PT_FIELDS+2)+4+8));else continue;return 1;}return 0;}
static const mpc_engine_t api={create,destroy,midi,set,get,render,NULL,NULL};
const mpc_engine_t *mpc_engine(void){return &api;}
