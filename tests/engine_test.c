#include <assert.h>
#include "../src/engine.c"
static long long energy(PT *p,int frames){int16_t out[256];long long e=0;while(frames){int n=frames>128?128:frames;render(p,out,n);for(int k=0;k<n*2;k++)e+=(long long)out[k]*out[k];frames-=n;}return e;}
static void note(PT*p,int n,int vel){uint8_t m[]={0x90,n,vel};midi(p,m,3);}
static float param(PT*p,const char*k){char s[64];assert(get(p,k,s,sizeof s)>0);return strtof(s,0);}
int main(void){
 PT*p=create(0),*q=create(0);assert(p&&q);assert(energy(p,128)==0);
 for(int i=0;i<8;i++){note(p,36+i,100);assert(p->v[i].active);assert(p->selected==0);assert(energy(p,128)>0);}
 assert(energy(q,256)==0);set(p,"selected","6");note(p,36,100);assert(p->selected==6);
 note(q,90,100);assert(energy(q,128)==0);note(q,0,100);assert(energy(q,128)>0);
 uint8_t off[]={0x80,0,0};midi(q,off,3);assert(q->v[0].active);
 uint8_t stop[]={0xb0,123,0};midi(q,stop,3);energy(q,128);assert(energy(q,128)==0);
 set(q,"v0_mute","1");note(q,0,100);assert(energy(q,128)==0);set(q,"v0_mute","0");
 set(q,"v1_solo","1");note(q,0,100);assert(energy(q,128)==0);note(q,1,100);assert(energy(q,128)>0);
 set(q,"v1_solo","0");midi(q,stop,3);energy(q,128);
 for(int order=0;order<2;order++){set(q,"v0_choke","1");set(q,"v1_choke","1");note(q,order?0:1,100);note(q,order?1:0,100);assert(q->v[0].active);assert(!q->v[1].active||q->v[1].release);energy(q,128);assert(!q->v[1].active);}
 char saved[32768],check[32768],text[128];
 for(int i=0;i<8;i++){char k[64],s[32];snprintf(k,sizeof k,"select%d",i);set(p,k,"1");snprintf(s,sizeof s,"%d",100+i);set(p,"q0_0",s);set(p,"compare","1");snprintf(s,sizeof s,"%d",300+i);set(p,"q0_0",s);}
 assert(get(p,"state",saved,sizeof saved)>0);set(q,"state",saved);assert(param(q,"state_valid")==1);assert(get(q,"state",check,sizeof check)>0);assert(!strcmp(saved,check));
 set(q,"state","PT1 0 nan");assert(param(q,"state_valid")==0);get(q,"state",check,sizeof check);assert(!strcmp(saved,check));
 set(p,"v7_level","65");set(p,"v7_pan","-34");set(p,"v7_choke","2");set(p,"v7_mute","1");set(p,"random","1");assert(param(p,"v7_level")==65&&param(p,"v7_pan")==-34&&param(p,"v7_choke")==2&&param(p,"v7_mute")==1);assert(param(p,"v6_freq")==306);
 get(p,"v0_modmode_display",text,sizeof text);assert(!strcmp(text,"DECAY"));
 destroy(p);destroy(q);
 p=create(0);q=create(0);note(p,36,127);note(q,36,127);int16_t a[256],b[256];render(p,a,128);render(q,b,17);render(q,b+34,111);assert(!memcmp(a,b,sizeof a));
 for(int i=0;i<PT_FIELDS;i++){change(p,0,i,pt_fields[i].hi);note(p,36,127);assert(energy(p,128)>=0);change(p,0,i,pt_fields[i].lo);note(p,36,1);energy(p,128);}
 destroy(p);destroy(q);puts("PASS: eight voices, routing, silence, mute/solo, choke, A/B state, rollback, random protection, block slicing and range smoke tests");
}
