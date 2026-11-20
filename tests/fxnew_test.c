/* πToniC 1.1.0 : effets NEW (Tidal Verb + Dub Station). */
#include <assert.h>
#include "../src/engine.c"
static void render_n(PT*p,int n,int *peak){int16_t out[256];while(n){int k=n>128?128:n;render(p,out,k);for(int i=0;i<2*k;i++)if(abs(out[i])>*peak)*peak=abs(out[i]);n-=k;}}
static float readp(PT*p,const char*k){char b[64];assert(get(p,k,b,sizeof b)>0);return strtof(b,0);}
int main(void){char s[32768],t[32768];int pk=0;
 PT*p=create(0);assert(p&&p->fxn&&p->fx_new==1);
 /* Sans send, aucun calcul d'effet. */
 trigger(p,0,127);render_n(p,4410,&pk);assert(!fxn_busy(p->fxn));
 /* Reverb seule puis delay seul : chacun produit une queue après la fin de la voix. */
 set(p,"v0_decay","10");set(p,"send0_reverb","100");trigger(p,0,127);render_n(p,4410,&pk);assert(fxn_busy(p->fxn));
 int tail=0;render_n(p,22050,&tail);assert(tail>0);
 set(p,"send0_reverb","0");render_n(p,44100*10,&tail);assert(!fxn_busy(p->fxn)); /* repos après 2 s de silence */
 set(p,"send0_delay","100");set(p,"fxn_dsync","0");set(p,"fxn_dms","200");trigger(p,0,127);render_n(p,2205,&pk);tail=0;render_n(p,22050,&tail);assert(tail>0);
 /* Balayages de tous les réglages pendant le jeu : sortie finie, pas de plantage. */
 set(p,"send0_reverb","100");
 for(int i=0;i<600;i++){char v[16];sprintf(v,"%d",i%10);set(p,"fxn_rtype",v);set(p,"fxn_rdecay",i%2?"100":"0");set(p,"fxn_dfeedback",i%3?"120":"0");set(p,"fxn_dfilter",i%2?"-100":"100");set(p,"fxn_dsync",i%4?"1":"0");sprintf(v,"%d",i%7);set(p,"fxn_ddiv",v);set(p,"rev_on",i%5?"1":"0");if(i%30==0)trigger(p,0,127);if(i%50==0)set(p,"fxn_mode",i%100?"0":"1");render_n(p,128,&pk);}
 set(p,"fxn_mode","1");
 /* Bornes, alias TIME et Q-Links. */
 set(p,"fxn_dms","99999");assert(readp(p,"fxn_dms")==2000);set(p,"fxn_rtype","3.4");assert(readp(p,"fxn_rtype")==3);
 set(p,"fxn_dsync","1");set(p,"fxn_time","0.5");assert(readp(p,"fxn_ddiv")==3);
 get(p,"fxq0_name",t,64);assert(!strcmp(t,"DECAY"));set(p,"fxq0","1");assert(readp(p,"fxn_rdecay")==100);get(p,"fxq0_display",t,64);assert(!strcmp(t,"100 %"));
 set(p,"fxn_mode","0");get(p,"fxq0_name",t,64);assert(!strcmp(t,"SIZE"));set(p,"fxq0","0");assert(readp(p,"rev_size")==0);set(p,"fxn_mode","1");
 /* État : aller-retour exact, rejet des valeurs invalides, ancien état -> CLASSIC. */
 get(p,"state",s,sizeof s);assert(strstr(s,"FX2 1 "));PT*q=create(0);set(q,"state",s);assert(q->state_valid);get(q,"state",t,sizeof t);assert(!strcmp(s,t));
 char bad[32768];strcpy(bad,s);strcpy(strstr(bad,"FX2"),"FX2 1 12");set(q,"state",bad);assert(!q->state_valid);get(q,"state",t,sizeof t);assert(!strcmp(s,t));
 PT*o=create(0);set(o,"fxn_mode","0");get(o,"state",t,sizeof t);assert(!strstr(t,"FX2"));set(q,"state",t);assert(q->state_valid&&q->fx_new==0);
 /* Notifications : changer le mode annonce le mode et les 8 Q-Links. */
 char key[64];while(mpc_engine_changed(q,key,sizeof key));set(q,"fxn_mode","1");int seen=0;while(mpc_engine_changed(q,key,sizeof key))seen+=!strcmp(key,"fxn_mode")||!strncmp(key,"fxq",3);assert(seen==9);
 destroy(p);destroy(q);destroy(o);
 puts("PASS NEW FX: repos sans send, queues reverb/delay, mise en veille, balayages, bornes, TIME/Q-Links, état FX2, ancien état -> CLASSIC, notifications");}
