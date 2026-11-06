#include <assert.h>
#include "../src/engine.c"
static void advance_fx(Effects*e,int n,float impulse,float*energy){for(int i=0;i<n;i++){float in[2]={i==0?impulse:0,0},zero[2]={0},out[2];fx_process(e,zero,in,out);for(int k=0;k<2;k++){assert(isfinite(out[k]));*energy+=out[k]*out[k];}}}
static void render_n(PT*p,int n){int16_t out[256];while(n){int k=n>128?128:n;render(p,out,k);n-=k;}}
static float readp(PT*p,const char*k){char b[64];assert(get(p,k,b,sizeof b)>0);return strtof(b,0);}
int main(void){
 Effects *e=calloc(1,sizeof *e);fx_init(e);e->param[DEL_SYNC]=0;e->param[DEL_MS]=20;e->param[DEL_FEEDBACK]=0;e->param[DEL_RETURN]=100;e->ready=0;fx_cache(e);float energy=0;advance_fx(e,881,1,&energy);assert(energy==0);advance_fx(e,4,0,&energy);assert(energy>.9f);free(e);
 e=calloc(1,sizeof *e);fx_init(e);float re=0;for(int i=0;i<220500;i++){float in[2]={i==0?.5f:0,i==0?.25f:0},zero[2]={0},out[2];fx_process(e,in,zero,out);for(int k=0;k<2;k++){assert(isfinite(out[k]));re+=out[k]*out[k];}}assert(re>0);free(e);
 PT*p=create(0),*q=create(0);char state[32768],other[32768];assert(p&&q);get(p,"state",state,sizeof state);assert(!strstr(state,"FX1"));
 set(p,"send3_reverb","67");set(p,"send7_delay","42");set(p,"rev_decay","6.2");set(p,"del_sync","0");set(p,"del_ms","480");set(p,"del_feedback","90");get(p,"state",state,sizeof state);assert(strstr(state,"FX1"));set(q,"state",state);assert(q->state_valid);get(q,"state",other,sizeof other);assert(!strcmp(state,other));assert(readp(q,"send3_reverb")==67&&readp(q,"send7_delay")==42);
 char bad[32768];strcpy(bad,state);char *f=strstr(bad,"FX1");strcpy(f,"FX1 nan");set(q,"state",bad);assert(!q->state_valid);get(q,"state",other,sizeof other);assert(!strcmp(state,other));
 set(q,"del_sync","1");set(q,"del_div","3");set(q,"lfo_bpm","120");assert(fabsf(q->fx.delay_target-22050)<1);set(q,"lfo_bpm","60");assert(fabsf(q->fx.delay_target-44100)<1);set(q,"fx_time",".5");assert(readp(q,"del_div")==2);for(int i=0;i<6;i++)set(q,"fx_time",".5078125");assert(readp(q,"del_div")==3);set(q,"del_sync","0");set(q,"fx_time","1");assert(readp(q,"del_ms")==1500);
 set(p,"send0_delay","100");set(p,"send0_reverb","100");set(p,"v0_decay","10");set(p,"v0_mix","0");set(p,"v0_modamt","0");trigger(p,0,127);render_n(p,4410);assert(p->fx.rev_left&&p->fx.del_left);assert(q->fx.rev_left==0&&q->fx.del_left==0);
 for(int i=0;i<1000;i++){set(p,"rev_size",i%2?"100":"0");set(p,"rev_decay",i%2?"8":".2");set(p,"del_ms",i%2?"1500":"20");set(p,"rev_on",i%3?"1":"0");set(p,"del_on",i%3?"0":"1");if(i%40==0)trigger(p,0,127);render_n(p,128);}
 /* Old sound state clears sends and restores defaults without changing its serialized recipe. */
 PT*r=create(0);get(r,"state",other,sizeof other);set(p,"state",other);assert(p->state_valid&&readp(p,"send0_delay")==0&&readp(p,"send0_reverb")==0);get(p,"state",state,sizeof state);assert(!strcmp(state,other));assert(!p->fx.rev_left&&!p->fx.del_left);destroy(p);destroy(q);destroy(r);
 puts("PASS: wet delay onset, reverb tail, finite feedback/control sweeps, tempo and contextual TIME, FX state round-trip and rejection, legacy dry presets, instance isolation");
}
