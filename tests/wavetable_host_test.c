/* πToniC 1.2.0 : SOURCE à 3 valeurs et Q-Links WAVETABLE à travers le vrai wrapper VST2 (valeurs normalisées de l'hôte). */
#define _GNU_SOURCE
#include <assert.h>
#include "../third_party/mpc-wrapper/wrapper/vst2_wrap.c"
static char names[4][256];static int updates;
static int idx(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return 0;}
static void read_names(AEffect*a){const char*k[]={"tone_q0","tone_q1","tone_q2","tone_q3"};for(int i=0;i<4;i++)a->dispatcher(a,effGetParamName,idx(k[i]),0,names[i],0);}
static intptr_t host(AEffect*a,int32_t op,int32_t i,intptr_t v,void*p,float f){(void)i;(void)v;(void)p;(void)f;if(op==audioMasterUpdateDisplay){updates++;read_names(a);}return 0;}
static void pump(AEffect*a,int count){float l[128],r[128],*out[]={l,r};for(int k=0;k<count;k++)a->processReplacing(a,0,out,128);}
static float raw(AEffect*a,const char*k){char b[128];assert(eng_get(a->object,k,b,sizeof b)>0);return strtof(b,0);}
int main(void){AEffect*a=VSTPluginMain(host);assert(a&&a->numParams==NPARAMS);a->dispatcher(a,effOpen,0,0,0,0);pump(a,160);
 /* normalisé 0,5 = WAVETABLE ; 1 = SAMPLE comme avant la 1.2.0 ; 0 = SYNTH */
 a->setParameter(a,idx("v0_source"),.5f);pump(a,100);read_names(a);assert(!strcmp(names[0],"FREQUENCY")&&!strcmp(names[1],"POS")&&!strcmp(names[2],"END")&&!strcmp(names[3],"SCAN TIME"));
 assert(fabsf(a->getParameter(a,idx("v0_source"))-.5f)<1e-6f);
 a->setParameter(a,idx("tone_q2"),.4f);a->setParameter(a,idx("tone_q3"),.75f);pump(a,80);assert(fabsf(raw(a,"v0_wt_end")-40)<.01f&&fabsf(raw(a,"v0_wt_time")-1000)<.5f);
 a->setParameter(a,idx("v0_source"),1);pump(a,100);read_names(a);assert(!strcmp(names[0],"TUNE")&&!strcmp(names[1],"START")&&!strcmp(names[2],"AMP ATTACK"));
 a->setParameter(a,idx("v0_source"),0);pump(a,100);read_names(a);assert(!strcmp(names[0],"FREQUENCY")&&!strcmp(names[1],"WAVE MORPH"));
 /* réglages par voix : TABLE et SCAN sont des listes, SCAN TIME suit une courbe logarithmique */
 a->setParameter(a,idx("v2_wt_table"),1);a->setParameter(a,idx("v2_wt_scan"),.5f);pump(a,40);assert(raw(a,"v2_wt_table")==7&&raw(a,"v2_wt_scan")==1);
 char text[256];a->dispatcher(a,effGetParamDisplay,idx("v2_wt_table"),0,text,0);assert(strstr(text,"NOISE"));
 a->setParameter(a,idx("v2_wt_time"),.5f);pump(a,40);float t=raw(a,"v2_wt_time");assert(t>50&&t<200);
 a->dispatcher(a,effClose,0,0,0,0);puts("PASS: wrapper SOURCE 3-way (0 / 0.5 / 1), POS / END / SCAN TIME names and values, per-voice TABLE / SCAN lists, SCAN TIME taper");return 0;}
