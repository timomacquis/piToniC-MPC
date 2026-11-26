/* πToniC 1.2.0 : source WAVETABLE — tables, balayage, paramètres, Q-Links, état et compatibilité. */
#include <assert.h>
#include "../src/engine.c"
static float param(PT*p,const char*k){char s[64];assert(get(p,k,s,sizeof s)>0);return strtof(s,0);}
static void text(PT*p,const char*k,char*s,int n){assert(get(p,k,s,n)>0);}
static long long energy(PT *p,int frames,int *bad){int16_t out[256];long long e=0;while(frames){int n=frames>128?128:frames;render(p,out,n);for(int k=0;k<n*2;k++){e+=(long long)out[k]*out[k];if(out[k]==32767||out[k]==-32767)(*bad)++;}frames-=n;}return e;}
static void drain(PT*p){char k[80];for(int i=0;i<20000&&mpc_engine_changed(p,k,sizeof k);i++);}
/* amplitude de l'harmonique h dans une période de WT_SIZE points */
static double harmonic(const float *d,int h){double re=0,im=0;for(int i=0;i<WT_SIZE;i++){re+=d[i]*cos(2*WT_PI*h*i/WT_SIZE);im+=d[i]*sin(2*WT_PI*h*i/WT_SIZE);}return sqrt(re*re+im*im)/WT_SIZE;}
int main(void){
 PT*p=create(0),*q=create(0);assert(p&&q);char s[128];
 /* 1. Tables : finies, crête <= 1, sans DC, limitées en harmoniques par version. */
 for(int t=0;t<WT_TABLES;t++)for(int f=0;f<WT_FRAMES;f++)for(int l=0;l<WT_LEVELS;l++){const float *d=wt_data[t][f][l];double dc=0,peak=0;
  for(int i=0;i<=WT_SIZE;i++){assert(isfinite(d[i]));if(fabs(d[i])>peak)peak=fabs(d[i]);}for(int i=0;i<WT_SIZE;i++)dc+=d[i];assert(d[WT_SIZE]==d[0]);
  assert(peak<=1.0001);assert(fabs(dc/WT_SIZE)<1e-4);
  int top=(WT_SIZE/2)>>l;if(top>=WT_SIZE/2)top=WT_SIZE/2-1;if(top+1<WT_SIZE/2&&(f==0||f==WT_FRAMES-1))assert(harmonic(d,top+1)<1e-5);}
 for(int t=0;t<WT_TABLES;t++){double pk=0;for(int f=0;f<WT_FRAMES;f++)for(int l=0;l<WT_LEVELS;l++)for(int i=0;i<WT_SIZE;i++)if(fabs(wt_data[t][f][l][i])>pk)pk=fabs(wt_data[t][f][l][i]);assert(fabs(pk-.95)<1e-3);}
 /* version choisie : harmonique la plus haute sous Nyquist */
 assert(wt_level(55.f/SR)==0);assert(wt_level(1000.f/SR)==4);assert(wt_level(.45f)==WT_LEVELS-1);
 for(int hz=20;hz<20000;hz=hz*11/10+1){int l=wt_level(hz/SR);int top=(WT_SIZE/2)>>l;if(top>=WT_SIZE/2)top=WT_SIZE/2-1;assert(top*hz<=SR/2||l==WT_LEVELS-1);}
 /* balayage */
 assert(wt_scan_shape(0,.5f)==.5f&&wt_scan_shape(0,3)==1);assert(fabsf(wt_scan_shape(1,1.25f)-.25f)<1e-6);assert(fabsf(wt_scan_shape(2,1.25f)-.75f)<1e-6);
 /* 2. SOURCE à 3 valeurs : 0 SYNTH, 1 WAVETABLE, 2 SAMPLE (valeur haute inchangée). */
 set(p,"v0_source","1");assert(p->source[0]==2&&param(p,"v0_source")==1);
 set(p,"v0_source","2");assert(p->source[0]==1&&param(p,"v0_source")==2);
 set(p,"v0_source","0");assert(p->source[0]==0&&param(p,"v0_source")==0);
 set(p,"source","1");assert(p->source[0]==2&&param(p,"source")==1);
 /* 3. Q-Links de TONE : en WAVETABLE FREQUENCY, POS, END, SCAN TIME, AMP ATTACK, AMP DECAY, PITCH AMOUNT, PITCH TIME / RATE, PITCH VELOCITY. */
 {const char *want[]={"FREQUENCY","POS","END","SCAN TIME","AMP ATTACK","AMP DECAY","PITCH AMOUNT","PITCH TIME / RATE","PITCH VELOCITY"};
  for(int i=0;i<9;i++){char k[32];snprintf(k,sizeof k,"tone_q%d_name",i);text(p,k,s,sizeof s);assert(!strcmp(s,want[i]));}}
 set(p,"tone_q2","0.25");assert(p->wt_end[0][0]==25);set(p,"tone_q3","0.5");assert(fabsf(p->wt_time[0][0]-100)<.01f);
 text(p,"tone_q3_display",s,sizeof s);assert(!strcmp(s,"100.00 ms"));assert(fabsf(param(p,"tone_q3")-.5f)<1e-5);
 set(p,"tone_q8","0.5");assert(fabsf(param(p,"v0_modvel")-100)<.01f);set(p,"tone_q4","0");assert(param(p,"v0_attack")==0);
 /* SYNTH et SAMPLE : ordre d'avant, 8e et 9e inactifs */
 set(p,"source","0");{const char *want[]={"FREQUENCY","WAVE MORPH","AMP ATTACK","AMP DECAY","PITCH AMOUNT","PITCH TIME / RATE","PITCH VELOCITY","—","—"};
  for(int i=0;i<9;i++){char k[32];snprintf(k,sizeof k,"tone_q%d_name",i);text(p,k,s,sizeof s);assert(!strcmp(s,want[i]));}}
 {float m=param(p,"v0_modvel");set(p,"tone_q7","0.9");set(p,"tone_q8","0.1");assert(p->wt_end[0][0]==25&&param(p,"v0_modvel")==m);}
 set(p,"source","2");text(p,"tone_q0_name",s,sizeof s);assert(!strcmp(s,"TUNE"));text(p,"tone_q2_name",s,sizeof s);assert(!strcmp(s,"AMP ATTACK"));set(p,"source","0");
 /* 4. Réglages par voix, par banque A/B. */
 set(p,"v3_wt_table","4");set(p,"v3_wt_end","60");set(p,"v3_wt_time","35");set(p,"v3_wt_scan","2");
 text(p,"v3_wt_table_display",s,sizeof s);assert(!strcmp(s,"FOLD"));text(p,"v3_wt_scan_display",s,sizeof s);assert(!strcmp(s,"PING-PONG"));
 assert(param(p,"v3_wt_end")==60&&param(p,"v3_wt_time")==35);set(p,"v3_wt_table","99");assert(p->wt_table[3][0]==7);
 set(p,"select3","1");set(p,"compare","1");assert(p->v[3].bank==1&&param(p,"v3_wt_table")==0);set(p,"v3_wt_table","2");set(p,"compare","1");assert(param(p,"v3_wt_table")==7);
 /* 5. Son : chaque table sonne, sans saturation, sur des hauteurs graves et aiguës ; SYNTH inchangé ailleurs. */
 for(int t=0;t<WT_TABLES;t++)for(int mode=0;mode<3;mode++)for(int hz=40;hz<=8000;hz*=5){PT*r=create(0);char v[32];set(r,"v0_source","1");snprintf(v,sizeof v,"%d",t);set(r,"v0_wt_table",v);snprintf(v,sizeof v,"%d",mode);set(r,"v0_wt_scan",v);
  snprintf(v,sizeof v,"%d",hz);set(r,"v0_freq",v);set(r,"v0_mix","0");set(r,"v0_drive","0");set(r,"v0_wt_time","30");set(r,"v0_wave","10");set(r,"v0_wt_end","90");
  trigger(r,0,127);int bad=0;long long e=energy(r,4410,&bad);assert(e>0&&bad==0);destroy(r);}
 /* le balayage change bien le son : POS 0 -> END 100 contre POS = END = 0 */
 {PT*a=create(0),*b=create(0);for(PT**x=(PT*[]){a,b,0};*x;x++){set(*x,"v0_source","1");set(*x,"v0_wt_table","3");set(*x,"v0_mix","0");}
  set(b,"v0_wt_end","0");trigger(a,0,127);trigger(b,0,127);int16_t oa[256],ob[256];int diff=0;for(int k=0;k<40;k++){render(a,oa,128);render(b,ob,128);diff|=memcmp(oa,ob,sizeof oa);}assert(diff);destroy(a);destroy(b);}
 /* 6. État : sans WAVETABLE, rien de neuf (identique à la 1.1.2) ; avec, bloc WT1 et aller-retour exact. */
 {PT*a=create(0);char st[32768];assert(get(a,"state",st,sizeof st)>0);assert(!strstr(st,"WT1")&&!strstr(st,"SM1"));destroy(a);}
 static char saved[32768],check[32768];assert(get(p,"state",saved,sizeof saved)>0);assert(strstr(saved,"WT1 "));
 set(q,"state",saved);assert(q->state_valid);assert(get(q,"state",check,sizeof check)>0);assert(!strcmp(saved,check));
 assert(q->wt_table[3][0]==7&&q->wt_table[3][1]==2&&q->wt_end[3][0]==60&&q->wt_scan[3][0]==2);
 set(q,"v5_source","1");assert(get(q,"state",saved,sizeof saved)>0);assert(strstr(saved,"SM1 ")&&strstr(saved,"WT1 "));
 /* WT1 invalide : refusé en entier, état précédent conservé */
 {char bad[32768];snprintf(bad,sizeof bad,"%s",saved);char *w=strstr(bad,"WT1 ");assert(w);w[4]='9';set(q,"state",bad);assert(!q->state_valid);get(q,"state",check,sizeof check);assert(!strcmp(saved,check));}
 /* état 1.1.2 (SM1 avec SAMPLE, sans WT1) : relu tel quel, réglages WAVETABLE par défaut */
 {PT*a=create(0);set(a,"v2_source","2");char st[32768];get(a,"state",st,sizeof st);assert(strstr(st,"SM1 ")&&!strstr(st,"WT1"));
  set(q,"state",st);assert(q->state_valid&&q->source[2]==1&&q->source[5]==0&&q->wt_table[3][0]==0&&q->wt_end[3][0]==WT_END_DEF&&q->wt_time[3][1]==WT_TIME_DEF);
  get(q,"state",check,sizeof check);assert(!strcmp(st,check));destroy(a);}
 /* 7. Import .mtpreset : toutes les voix passent en SYNTH, les réglages WAVETABLE restent (comme TUNE/START). */
 {MTKit k;memset(&k,0,sizeof k);for(int v=0;v<8;v++)memcpy(k.values[v],p->v[v].banks[0],sizeof k.values[v]);set(p,"v3_source","1");kit_apply(p,&k);
  for(int v=0;v<8;v++)assert(p->source[v]==0);assert(p->wt_table[3][0]==7&&p->wt_end[3][0]==60);}
 /* 8. Notifications : chaque réglage WAVETABLE modifié est signalé à l'hôte. */
 drain(p);set(p,"v6_wt_end","12");{char k[80];int seen=0;for(int i=0;i<200&&mpc_engine_changed(p,k,sizeof k);i++){if(!strcmp(k,"v6_wt_end"))seen|=1;if(!strcmp(k,"tone_q2"))seen|=2;}assert(seen==3);}
 /* 9. RAND VOICE en WAVETABLE garde POS dans la plage et tire END / SCAN TIME ; ALTER garde la table. */
 set(p,"select4","1");set(p,"v4_source","1");set(p,"random","1");assert(p->wt_end[4][p->v[4].bank]>=0&&p->wt_end[4][p->v[4].bank]<=100&&p->wt_time[4][p->v[4].bank]>=1);
 {int t=p->wt_table[4][p->v[4].bank];set(p,"alter","1");assert(p->wt_table[4][p->v[4].bank]==t);}
 destroy(p);destroy(q);
 puts("PASS: wavetable tables, band limits, scan, 3-way SOURCE mapping, Q-Links, A/B, sound, WT1 state, legacy state, .mtpreset import, notifications, random");
}
