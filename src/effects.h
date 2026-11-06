/* πToniC shared send effects, MIT, 2026 Timo.
 * Adapted architecture: Weird Dreams by Vincent Fillion (WeirdDrums, MIT).
 * Eight damped combs / four allpasses and filtered stereo cross-feedback delay.
 * Cached coefficients, interpolated delay reads, smoothed controls, no allocation in DSP.
 */
#pragma once
#define FX_RATE 44100.f
#define FX_RING 4096
#define FX_DELAY 264602 /* six seconds plus interpolation guard */
#define FX_COUNT 12
enum {REV_ON,REV_SIZE,REV_DECAY,REV_DAMP,REV_RETURN,DEL_ON,DEL_SYNC,DEL_DIV,DEL_MS,DEL_FEEDBACK,DEL_TONE,DEL_RETURN};
static const char *fx_keys[FX_COUNT]={"rev_on","rev_size","rev_decay","rev_damp","rev_return","del_on","del_sync","del_div","del_ms","del_feedback","del_tone","del_return"};
static const float fx_lo[FX_COUNT]={0,0,.2f,0,0,0,0,0,20,0,0,0};
static const float fx_hi[FX_COUNT]={1,100,8,100,100,1,1,4,1500,90,100,100};
static const float fx_defaults[FX_COUNT]={1,65,2.4f,45,55,1,1,2,375,35,55,50};
typedef struct {float buf[FX_RING],filter,length,target,feedback;unsigned pos;} FXComb;
typedef struct {float buf[512];unsigned pos,len;} FXAllpass;
typedef struct {
 FXComb comb[8];FXAllpass ap[4];float delay[2][FX_DELAY];unsigned pos;
 float param[FX_COUNT],bpm,delay_target,delay_time,damp,tone,feedback;
 float smooth[FX_COUNT],lp[2],rev_lp[2];
 uint64_t rev_left,del_left,delay_horizon;int ready;
} Effects;
static float fx_zap(float x){return fabsf(x)<1e-20f?0:x;}
static float fx_read(const float *b,unsigned pos,int length,float delay){
 float at=(float)pos-delay;if(at<0)at+=length;int i=(int)at;float f=at-i;int next=i+1;if(next==length)next=0;
 return b[i]+f*(b[next]-b[i]);
}
static void fx_cache(Effects *e){
 static const int lengths[8]={1117,1277,1499,1657,1153,1319,1531,1693};
 static const float beats[5]={.25f,.5f,.75f,1,2};
 for(int i=0;i<8;i++){e->comb[i].target=lengths[i]*(.5f+.8f*e->param[REV_SIZE]/100);e->comb[i].feedback=powf(.001f,e->comb[i].target/(FX_RATE*e->param[REV_DECAY]));}
 e->damp=.05f+.9f*(1-e->param[REV_DAMP]/100);
 e->tone=1-expf(-6.283185307f*(300+15700*e->param[DEL_TONE]/100)/FX_RATE);
 e->feedback=e->param[DEL_FEEDBACK]/100;
 float seconds=e->param[DEL_SYNC]>.5f?60/e->bpm*beats[(int)e->param[DEL_DIV]]:e->param[DEL_MS]/1000;
 e->delay_target=fminf(FX_DELAY-2,fmaxf(1,seconds*FX_RATE));
 float repeats=e->feedback>0?ceilf(logf(1e-8f)/logf(e->feedback)):1;
 e->delay_horizon=(uint64_t)((fmaxf(e->delay_time,e->delay_target)+2)*(repeats+2));
 if(e->del_left && e->del_left<e->delay_horizon)e->del_left=e->delay_horizon;
 if(!e->ready){for(int i=0;i<8;i++)e->comb[i].length=e->comb[i].target;e->delay_time=e->delay_target;memcpy(e->smooth,e->param,sizeof e->smooth);e->ready=1;}
}
static void fx_init(Effects *e){
 memset(e,0,sizeof *e);memcpy(e->param,fx_defaults,sizeof e->param);e->bpm=120;
 const unsigned len[4]={211,307,227,317};for(int i=0;i<4;i++)e->ap[i].len=len[i];fx_cache(e);
}
static float fx_allpass(FXAllpass *a,float in){float v=a->buf[a->pos],out=v-in;a->buf[a->pos]=fx_zap(in+v*.5f);if(++a->pos==a->len)a->pos=0;return out;}
static void fx_process(Effects *e,const float rev[2],const float del[2],float wet[2]){
 wet[0]=wet[1]=0;
 /* Controls remain cheap while idle, so enabling a previously unused send is smooth. */
 for(int i=0;i<FX_COUNT;i++)e->smooth[i]+=.002f*(e->param[i]-e->smooth[i]);
 const float rg=e->smooth[REV_ON],dg=e->smooth[DEL_ON];
 float ri[2]={rev[0]*rg,rev[1]*rg},di[2]={del[0]*dg,del[1]*dg};
 if(fabsf(ri[0])+fabsf(ri[1])>1e-12f)e->rev_left=(uint64_t)(FX_RATE*18); /* > two max RT60s */
 if(fabsf(di[0])+fabsf(di[1])>1e-12f)e->del_left=e->delay_horizon;
 if(e->rev_left){
  --e->rev_left;float sum[2]={0,0};
  for(int k=0;k<2;k++)e->rev_lp[k]=fx_zap(e->rev_lp[k]+.4f*(ri[k]-e->rev_lp[k]));
  for(int i=0;i<8;i++){FXComb *c=&e->comb[i];c->length+=.001f*(c->target-c->length);float y=fx_read(c->buf,c->pos,FX_RING,c->length);c->filter=fx_zap(c->filter+e->damp*(y-c->filter));c->buf[c->pos]=fx_zap(e->rev_lp[i/4]+c->filter*c->feedback);c->pos=(c->pos+1)&(FX_RING-1);sum[i/4]+=y*.25f;}
  for(int k=0;k<2;k++)wet[k]+=fx_allpass(&e->ap[k*2+1],fx_allpass(&e->ap[k*2],sum[k]))*e->smooth[REV_RETURN]/100*rg;
 }
 if(e->del_left){
  --e->del_left;e->delay_time+=.001f*(e->delay_target-e->delay_time);float read[2];
  for(int k=0;k<2;k++){read[k]=fx_read(e->delay[k],e->pos,FX_DELAY,e->delay_time);e->lp[k]=fx_zap(e->lp[k]+e->tone*(read[k]-e->lp[k]));wet[k]+=read[k]*e->smooth[DEL_RETURN]/100*dg;}
  for(int k=0;k<2;k++)e->delay[k][e->pos]=fx_zap(di[k]+e->lp[1-k]*e->feedback);
  if(++e->pos==FX_DELAY)e->pos=0;
 }
}
