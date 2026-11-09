#define _GNU_SOURCE
#include <assert.h>
#include "../third_party/mpc-wrapper/wrapper/vst2_wrap.c"
static intptr_t host(AEffect*a,int32_t op,int32_t i,intptr_t v,void*p,float f){(void)a;(void)op;(void)i;(void)v;(void)p;(void)f;return 0;}
static void state(AEffect*e,char*b){wrap_t*w=e->object;assert(eng_get(w,"state",b,32768)>0);}
int main(void){
 AEffect*a=VSTPluginMain(host),*b=VSTPluginMain(host);assert(a&&b&&a->numPrograms==PITONIC_FACTORY_COUNT&&b->numPrograms==PITONIC_FACTORY_COUNT);assert(a->numParams==418);char text[32768],other[32768],name[64];
 for(int i=0;i<PITONIC_FACTORY_COUNT;i++){
  memset(name,0,sizeof name);assert(a->dispatcher(a,effGetProgramNameIndexed,i,0,name,0)==1);assert(!strcmp(name,PITONIC_FACTORY[i].name));
  a->dispatcher(a,effSetProgram,0,i,0,0);assert(a->dispatcher(a,effGetProgram,0,0,0,0)==i);state(a,text);assert(!strcmp(text,PITONIC_FACTORY[i].state));
 }
 assert(a->dispatcher(a,effGetProgramNameIndexed,-1,0,name,0)==0);assert(a->dispatcher(a,effGetProgramNameIndexed,PITONIC_FACTORY_COUNT,0,name,0)==0);
 a->dispatcher(a,effSetProgram,0,17,0,0);a->setParameter(a,1,.37f);state(a,text);assert(strcmp(text,PITONIC_FACTORY[17].state));
 a->dispatcher(a,effSetProgram,0,17,0,0);state(a,other);assert(!strcmp(text,other)); /* host echo keeps edits */
 void*ptr=0;intptr_t size=a->dispatcher(a,effGetChunk,0,0,&ptr,0);assert(size>0);char*copy=malloc(size);memcpy(copy,ptr,size);assert(!strncmp(copy,"PTPROG1 17\n",11));
 assert(b->dispatcher(b,effSetChunk,0,size,copy,0)==1);assert(b->dispatcher(b,effGetProgram,0,0,0,0)==17);state(b,other);assert(!strcmp(text,other));free(copy);
 char bad[]="PTPROG1 116\nPT1 0 ";assert(b->dispatcher(b,effSetChunk,0,sizeof bad,bad,0)==0);assert(b->dispatcher(b,effGetProgram,0,0,0,0)==17);state(b,other);assert(!strcmp(text,other));
 const char*legacy=PITONIC_FACTORY[7].state;assert(b->dispatcher(b,effSetChunk,0,strlen(legacy)+1,(void*)legacy,0)==1);assert(b->dispatcher(b,effGetProgram,0,0,0,0)==7);state(b,other);assert(!strcmp(legacy,other));
 char removed[32768];snprintf(removed,sizeof removed,"PTPROG1 115\n%s",legacy);
 assert(b->dispatcher(b,effSetChunk,0,strlen(removed)+1,removed,0)==1);state(b,other);assert(!strcmp(legacy,other)); /* full sound survives a removed factory entry */
 a->dispatcher(a,effSetProgram,0,1,0,0);state(b,other);assert(!strcmp(legacy,other));
 a->dispatcher(a,effSetProgram,0,-1,0,0);assert(a->dispatcher(a,effGetProgram,0,0,0,0)==1);
 /* Actual VST chunk keeps sample configuration and original paths too. */
 wrap_t *wa=a->object;eng_set(wa,"source","1");eng_set(wa,"sample_path","/missing sample folder/metal.wav");eng_set(wa,"sample_tune","7");state(a,text);assert(strstr(text,"SM1 "));void *sptr=NULL;intptr_t ssize=a->dispatcher(a,effGetChunk,0,0,&sptr,0);assert(ssize>0);char *scopy=malloc(ssize);memcpy(scopy,sptr,ssize);assert(b->dispatcher(b,effSetChunk,0,ssize,scopy,0)==1);state(b,other);assert(!strcmp(text,other));free(scopy);
 a->dispatcher(a,effClose,0,0,0,0);b->dispatcher(b,effClose,0,0,0,0);
 puts("PASS: native program names and full-kit recalls; unchanged indices; edited-preset project round-trip with program index; same-program echo protection; legacy chunks; invalid-state rejection; instance independence.");
}
