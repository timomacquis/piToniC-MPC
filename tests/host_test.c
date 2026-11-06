#define _GNU_SOURCE
#include <assert.h>
#include "../third_party/mpc-wrapper/wrapper/vst2_wrap.c"
static int updates;
static VstTimeInfo clock_info={.tempo=120,.flags=kVstTempoValid};
static intptr_t host(AEffect*a,int32_t op,int32_t i,intptr_t v,void*p,float f){(void)a;(void)i;(void)v;(void)p;(void)f;if(op==42)updates++;if(op==audioMasterGetTime)return (intptr_t)&clock_info;return 0;}
int main(void){AEffect*a=VSTPluginMain(host),*b=VSTPluginMain(host);assert(a&&b&&a->numParams==NPARAMS);a->dispatcher(a,effOpen,0,0,0,0);b->dispatcher(b,effOpen,0,0,0,0);
 float l[128],r[128];float*out[]={l,r};a->processReplacing(a,0,out,128);for(int i=0;i<128;i++)assert(l[i]==0&&r[i]==0);
 VstMidiEvent m={.type=1,.byteSize=sizeof m,.deltaFrames=48,.midiData={0x90,36,127,0}};VstEvents ev={.numEvents=1,.events={(VstEvent*)&m}};a->dispatcher(a,effProcessEvents,0,0,&ev,0);char activity[16];eng_get(a->object,"activity0",activity,sizeof activity);assert(!strcmp(activity,"0"));a->processReplacing(a,0,out,128);double e=0;for(int i=0;i<128;i++){assert(isfinite(l[i])&&isfinite(r[i]));if(i<48)assert(l[i]==0&&r[i]==0);else e+=fabs(l[i])+fabs(r[i]);}assert(e>0);eng_get(a->object,"activity0",activity,sizeof activity);assert(!strcmp(activity,"1"));assert(updates==0);
 for(int i=0;i<NPARAMS;i++)if(PITONIC_CURVE[i])for(int j=0;j<=16;j++){char temp[64];float n=j/16.f;norm_to_str(&PARAMS[i],n,temp,sizeof temp);assert(fabsf(str_to_norm(&PARAMS[i],temp)-n)<.0001f);}
 void*chunk=0;intptr_t size=a->dispatcher(a,effGetChunk,0,0,&chunk,0);assert(size>0&&chunk);void*copy=malloc(size);memcpy(copy,chunk,size);b->dispatcher(b,effSetChunk,0,size,copy,0);free(copy);
 // UI selection and enum changes must not rebuild the screen on every write.
 for(int i=0;i<20;i++)a->processReplacing(a,0,out,128);updates=0;
 a->setParameter(a,1+4,1.0f);for(int i=0;i<24;i++)a->processReplacing(a,0,out,128);assert(updates==0);
 clock_info.tempo=160;a->processReplacing(a,0,out,128);assert(((wrap_t*)a->object)->bpm==160);
 for(int i=301;i<329;i++)a->setParameter(a,i,.37f);
 for(int i=0;i<40;i++)a->processReplacing(a,0,out,128);assert(updates==0);
 a->dispatcher(a,effClose,0,0,0,0);b->dispatcher(b,effClose,0,0,0,0);puts("PASS: actual wrapper, MIDI sample offset, chunks, enum notification without full rebuild, instance lifecycle");}
