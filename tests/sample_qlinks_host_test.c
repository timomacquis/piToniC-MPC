#define _GNU_SOURCE
#include <assert.h>
#include "../third_party/mpc-wrapper/wrapper/vst2_wrap.c"
static int updates,automations,peak,ordinary,ordinary_peak;static char names[2][64];
static int idx(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return 0;}
static intptr_t host(AEffect*a,int32_t op,int32_t i,intptr_t v,void*p,float f){(void)i;(void)v;(void)p;(void)f;if(op==audioMasterAutomate){automations++;if(!(i>=0&&i<NPARAMS&&PARAMS[i].momentary&&f==0))ordinary++;}if(op==audioMasterUpdateDisplay){updates++;a->dispatcher(a,effGetParamName,idx("tone_q0"),0,names[0],0);a->dispatcher(a,effGetParamName,idx("tone_q1"),0,names[1],0);}return 0;}
static void pump(AEffect*a,int count){float l[128],r[128],*out[]={l,r};for(int k=0;k<count;k++){int before=automations,normal_before=ordinary;a->processReplacing(a,0,out,128);int burst=automations-before;if(burst>peak)peak=burst;int normal=ordinary-normal_before;if(normal>ordinary_peak)ordinary_peak=normal;}}
static float raw(AEffect*a,const char*k){char b[128];assert(eng_get(a->object,k,b,sizeof b)>0);return strtof(b,0);}
int main(void){AEffect*a=VSTPluginMain(host);assert(a&&a->numParams==NPARAMS);a->dispatcher(a,effOpen,0,0,0,0);pump(a,160);updates=automations=peak=ordinary=ordinary_peak=0;
 a->dispatcher(a,effGetParamName,idx("tone_q0"),0,names[0],0);a->dispatcher(a,effGetParamName,idx("tone_q1"),0,names[1],0);assert(!strcmp(names[0],"FREQUENCY")&&!strcmp(names[1],"WAVE MORPH"));float frequency=raw(a,"v0_freq"),wave=raw(a,"v0_wave");
 a->setParameter(a,idx("v0_source"),1);pump(a,100);assert(updates==1&&!strcmp(names[0],"TUNE")&&!strcmp(names[1],"START"));
 a->setParameter(a,idx("tone_q0"),.75f);a->setParameter(a,idx("tone_q1"),.25f);pump(a,80);assert(fabsf(raw(a,"sample_tune")-24)<.001f&&fabsf(raw(a,"sample_start")-24.975f)<.001f);assert(raw(a,"v0_freq")==frequency&&raw(a,"v0_wave")==wave);assert(updates==1);
 for(int k=0;k<60;k++){a->setParameter(a,idx("tone_q0"),(k%11)/10.f);a->setParameter(a,idx("tone_q1"),(k%17)/16.f);pump(a,8);}pump(a,80);assert(updates==1);assert(ordinary_peak<=PICN_NOTIFICATION_BUDGET&&peak<=PICN_NOTIFICATION_BUDGET+1);
 a->setParameter(a,idx("v0_source"),1);pump(a,80);assert(updates==1); /* same source echo cannot rename/release again */
 a->setParameter(a,idx("select1"),1);pump(a,100);assert(updates==2&&!strcmp(names[0],"FREQUENCY")&&!strcmp(names[1],"WAVE MORPH"));a->setParameter(a,idx("tone_q0"),.6f);pump(a,80);assert(raw(a,"v1_freq")>100&&raw(a,"v0_freq")==frequency);assert(updates==2);
 a->setParameter(a,idx("select2"),1);pump(a,100);assert(updates==2); /* same SYNTH context */
 a->setParameter(a,idx("v2_source"),1);pump(a,100);assert(updates==3&&!strcmp(names[0],"TUNE")&&!strcmp(names[1],"START"));a->setParameter(a,idx("tone_q0"),.25f);pump(a,80);assert(fabsf(raw(a,"v2_sample_tune")+24)<.001f);assert(updates==3);
 a->setParameter(a,idx("v2_source"),0);pump(a,100);assert(updates==4&&!strcmp(names[0],"FREQUENCY"));assert(ordinary_peak<=PICN_NOTIFICATION_BUDGET&&peak<=PICN_NOTIFICATION_BUDGET+1);
 a->dispatcher(a,effClose,0,0,0,0);printf("PASS actual wrapper: physical TONE aliases switch TUNE/START and FREQUENCY/WAVE MORPH; host name cache refreshed exactly once per type change; same-type selection/echo and 120 Q-Link writes do not rebuild; peak callbacks %d including momentary reset, ordinary peak %d\n",peak,ordinary_peak);}
