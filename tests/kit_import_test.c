/* πToniC 1.1.1: .mtpreset kit import through the engine (set/get/render).
 * Original implementation, MIT. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include "../src/engine.c"

static void kit_file(const char *path,int broken){
 FILE *f=fopen(path,"w");assert(f);
 fprintf(f,"MicroTonicPresetV3: {\n\tTempo = 120\n\tMutes: { Off, Off, Off, Off, Off, Off, Off, Off }\n\tDrumPatches: {\n");
 for(int v=1;v<=8;v++)fprintf(f,"\t\t%d: {\n\t\t\tDistAmt = %s\n\t\t\tEQFreq = 900\n\t\t\tEQGain = 30\n\t\t\tLevel = %d\n\t\t\tMix = %d\n\t\t\tModAmt = 10\n\t\t\tModMode = Decay\n\t\t\tModRate = 80\n\t\t\tModVel = 0\n\t\t\tNEnvAtk = 0\n\t\t\tNEnvDcy = 90\n\t\t\tNEnvMod = %s\n\t\t\tNFilFrq = 3000\n\t\t\tNFilMod = HP\n\t\t\tNFilQ = 2\n\t\t\tNStereo = Off\n\t\t\tNVel = 20\n\t\t\tName = \"S%d\"\n\t\t\tOscAtk = 0\n\t\t\tOscDcy = 250\n\t\t\tOscFreq = %d\n\t\t\tOscVel = 30\n\t\t\tOscWave = Sine\n\t\t\tOutput = A\n\t\t\tPan = %d\n\t\t}\n",
  v,broken&&v==5?"x1":"0",v==1?6:-v,10*v,v==8?"Mod":"Exp",v,40+10*v,v*5);
 fprintf(f,"\t}\n}\n");fclose(f);
}
static void touch(const char *path){FILE *f=fopen(path,"w");assert(f);fputs("x",f);fclose(f);}
static void listed(PT *p,const char *dir,int filter,int count){
 for(int t=0;t<1000;t++){int ok;pthread_mutex_lock(&p->samples->mutex);ok=!strcmp(p->samples->directory,dir)&&p->samples->listing==filter&&p->samples->count==count;pthread_mutex_unlock(&p->samples->mutex);if(ok)return;struct timespec ts={0,2000000};nanosleep(&ts,NULL);}
 fprintf(stderr,"listing %s/%d/%d not reached\n",dir,filter,count);assert(0);
}
static void wait_kit(PT *p,int status){int16_t out[256];for(int t=0;t<1000;t++){render(p,out,128);if(atomic_load(&p->samples->kit_status)==status&&!atomic_load(&p->samples->kit_pending))return;struct timespec ts={0,2000000};nanosleep(&ts,NULL);}fprintf(stderr,"kit status %d != %d\n",atomic_load(&p->samples->kit_status),status);assert(0);}
static void text(PT *p,const char *key,char *buf,int len){assert(get(p,key,buf,len)>0);}

int main(void){
 char dir[]="/tmp/pitonic-kit-XXXXXX";assert(mkdtemp(dir));char sub[SP_PATH],kit[SP_PATH],bad[SP_PATH],wav[SP_PATH],nested[SP_PATH];
 snprintf(sub,sizeof sub,"%s/More kits",dir);assert(!mkdir(sub,0700));
 snprintf(kit,sizeof kit,"%s/Kit Ü (90).mtpreset",dir);kit_file(kit,0);
 snprintf(bad,sizeof bad,"%s/Broken.MTPRESET",dir);kit_file(bad,1);
 snprintf(wav,sizeof wav,"%s/hit.wav",dir);touch(wav);snprintf(nested,sizeof nested,"%s/inner.mtpreset",sub);kit_file(nested,0);
 MTKit want;assert(mt_load(kit,&want));
 PT *p=create(NULL);char buf[512];
 /* State the import must replace or keep. */
 set(p,"send2_reverb","40");set(p,"send5_delay","70");set(p,"v3_mute","1");set(p,"v6_solo","1");set(p,"v4_source","1");
 set(p,"selected","1");set(p,"compare","1");assert(p->v[1].bank==1);set(p,"rev_return","33");set(p,"fxn_mode","1");
 /* IMPORT KIT opens the kit browser, not the sample one. */
 set(p,"kit_import","1");text(p,"kit_browse",buf,sizeof buf);assert(!strcmp(buf,"1"));text(p,"browse",buf,sizeof buf);assert(!strcmp(buf,"0"));
 text(p,"kit_status_display",buf,sizeof buf);assert(strstr(buf,".mtpreset"));
 set(p,"browse_path_set",dir);listed(p,dir,1,3); /* [DIR] More kits, Broken.MTPRESET, Kit Ü (90).mtpreset — no .wav */
 text(p,"browse_item0_display",buf,sizeof buf);assert(!strcmp(buf,"[DIR] More kits"));
 text(p,"browse_item2_display",buf,sizeof buf);assert(!strcmp(buf,"Kit Ü (90).mtpreset"));
 /* Choosing a kit closes the browser; render applies it. */
 set(p,"browse_item2","1");text(p,"kit_browse",buf,sizeof buf);assert(!strcmp(buf,"0"));
 wait_kit(p,2);
 for(int v=0;v<8;v++){Voice *voice=&p->v[v];
  for(int b=0;b<2;b++)for(int f=0;f<PT_FIELDS;f++)assert(voice->banks[b][f]==want.values[v][f]);
  assert(voice->bank==0&&!voice->mute&&!voice->solo&&!p->source[v]&&p->sends[v][0]==0&&p->sends[v][1]==0);}
 assert(p->fx.param[REV_RETURN]==33&&p->fx_new==1);
 assert(p->v[0].banks[0][LEVEL]==100&&p->v[7].banks[0][ENV]==2&&p->v[0].banks[0][EQG]==30);
 text(p,"kit_status_display",buf,sizeof buf);assert(!strcmp(buf,"Imported: Kit Ü (90)"));
 /* Host notifications include voices, sends, sources and the status line. */
 {char key[80];int seen=0;for(int k=0;k<5000&&mpc_engine_changed(p,key,sizeof key);k++){if(!strcmp(key,"v7_env"))seen|=1;if(!strcmp(key,"send2_reverb"))seen|=2;if(!strcmp(key,"v4_source"))seen|=4;if(!strcmp(key,"kit_status"))seen|=8;if(!strcmp(key,"v3_mute"))seen|=16;}assert(seen==31);}
 /* The imported kit is an ordinary state: save / reload identical. */
 char saved[32768],check[32768];assert(get(p,"state",saved,sizeof saved)>0);PT *q=create(NULL);set(q,"state",saved);assert(q->state_valid);assert(get(q,"state",check,sizeof check)>0&&!strcmp(saved,check));destroy(q);
 /* A broken file changes nothing and says why. */
 float before[8][2][PT_FIELDS];for(int v=0;v<8;v++)memcpy(before[v],p->v[v].banks,sizeof before[v]);set(p,"v0_decay","777");before[0][0][DCY]=777;
 set(p,"kit_import","1");listed(p,dir,1,3);set(p,"browse_item1","1");wait_kit(p,3);
 for(int v=0;v<8;v++)assert(!memcmp(before[v],p->v[v].banks,sizeof before[v]));
 text(p,"kit_status_display",buf,sizeof buf);assert(!strcmp(buf,"Sound 5: invalid DistAmt — Broken"));
 /* Directory navigation stays in the kit filter; CLOSE cancels. */
 set(p,"kit_import","1");listed(p,dir,1,3);set(p,"browse_item0","1");listed(p,sub,1,1);set(p,"browse_up","1");listed(p,dir,1,3);
 set(p,"browse_close","1");text(p,"kit_browse",buf,sizeof buf);assert(!strcmp(buf,"0"));
 /* LOAD SAMPLE lists .wav again and remembers its own folder. */
 set(p,"load","1");text(p,"browse",buf,sizeof buf);assert(!strcmp(buf,"1"));text(p,"kit_browse",buf,sizeof buf);assert(!strcmp(buf,"0"));
 set(p,"browse_path_set",dir);listed(p,dir,0,2);text(p,"browse_item1_display",buf,sizeof buf);assert(!strcmp(buf,"hit.wav"));
 set(p,"browse_item0","1");listed(p,sub,0,0);set(p,"browse_close","1");
 set(p,"kit_import","1");listed(p,dir,1,3); /* back in the last kit folder */
 /* LOAD SAMPLE reopens its own last folder. */
 set(p,"browse_close","1");set(p,"load","1");listed(p,sub,0,0);set(p,"browse_close","1");
 /* Teardown with a pending, never-applied kit. */
 set(p,"kit_import","1");listed(p,dir,1,3);set(p,"browse_item2","1");
 for(int t=0;t<500&&atomic_load(&p->samples->kit_status)!=2;t++){struct timespec ts={0,2000000};nanosleep(&ts,NULL);}
 destroy(p);
 unlink(kit);unlink(bad);unlink(wav);unlink(nested);rmdir(sub);rmdir(dir);
 puts("PASS KIT IMPORT: .mtpreset browser filter and folders, worker read, render apply (A+B, SYNTH, mutes/solos, bank A, sends 0, FX kept), notifications, state roundtrip, broken file rollback, teardown");
}
