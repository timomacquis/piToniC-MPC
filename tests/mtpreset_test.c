/* πToniC .mtpreset reader tests. Original implementation, MIT.
 *   mtpreset-test            robustness and conversion unit tests
 *   mtpreset-test --dump F   print the converted kit of F (used by
 *                            tests/mtpreset_compare.py) */
#include <assert.h>
#include "mtpreset.h"

static int failures;
#define CHECK(c,...) do{if(!(c)){failures++;printf("FAIL line %d: ",__LINE__);printf(__VA_ARGS__);printf("\n");}}while(0)

static const char *sound_fields=
 "DistAmt = 20\nEQFreq = 500\nEQGain = 3\nLevel = %s\nMix = %s\nModAmt = 12\nModMode = Decay\nModRate = 100\n"
 "ModVel = 10\nNEnvAtk = 1\nNEnvDcy = 100\nNEnvMod = %s\nNFilFrq = 2000\nNFilMod = BP\nNFilQ = %s\nNStereo = On\n"
 "NVel = 50\nName = \"Snd\"\nOscAtk = 1\nOscDcy = 300\nOscFreq = 60\nOscVel = 40\nOscWave = Triangle\nOutput = %s\nPan = -10\n%s";

/* A full preset; sound 1 can be customised, sounds 2-8 are plain. */
static char *preset(const char *level,const char *mix,const char *env,const char *q,const char *output,const char *extra,const char *tail){
 size_t cap=64*1024;char *s=malloc(cap);int n=snprintf(s,cap,"MicroTonicPresetV3: {\n\tTempo = 120\n\tMutes: { Off, Off, Off, Off, Off, Off, Off, Off }\n\tDrumPatches: {\n");
 for(int v=1;v<=8;v++){n+=snprintf(s+n,cap-n,"\t\t%d: {\n",v);n+=v==1?snprintf(s+n,cap-n,sound_fields,level,mix,env,q,output,extra):snprintf(s+n,cap-n,sound_fields,"0","50","Exp","1","A","");n+=snprintf(s+n,cap-n,"\t\t}\n");}
 n+=snprintf(s+n,cap-n,"\t}\n\tPatterns: {\n\t\ta: {\n\t\t\tLength = 16\n\t\t\t1: {\n\t\t\t\tTriggers = #---#---#---#---\n\t\t\t}\n\t\t}\n\t}\n}\n%s",tail);
 return s;
}
static int parse(const char *text,MTKit *kit){return mt_parse(text,strlen(text),kit);}
static int expect_error(const char *text,const char *fragment){
 MTKit kit;memset(&kit,0x5a,sizeof kit);MTKit before=kit;
 int ok=parse(text,&kit);
 CHECK(!ok,"accepted, expected error containing '%s'",fragment);
 CHECK(strstr(kit.message,fragment)!=NULL,"message '%s' lacks '%s'",kit.message,fragment);
 CHECK(!memcmp(kit.values,before.values,sizeof kit.values),"values changed on error '%s'",fragment);
 return ok;
}
static float field(const MTKit *k,int v,int f){return k->values[v][f];}

static void unit_tests(void){
 MTKit kit;char *s;
 /* Plain valid preset, numeric Mix (oscillator share) and Choke absent. */
 s=preset("0","50","Exp","1","A","","");CHECK(parse(s,&kit),"valid preset refused: %s",kit.message);free(s);
 CHECK(field(&kit,0,8)==50,"mix");CHECK(field(&kit,0,1)==strtof("33.3333333",NULL),"wave %g",field(&kit,0,1));
 CHECK(field(&kit,0,16)==10,"drive");CHECK(field(&kit,0,19)==100,"level");CHECK(field(&kit,0,23)==0,"choke default");
 CHECK(field(&kit,0,15)==1&&field(&kit,0,9)==1,"options");CHECK(kit.clamped==0&&kit.clap==0&&kit.outputs==0,"counters");
 /* Ratio Mix, Mod envelope, other output, choke, positive level attenuation. */
 s=preset("6","1/3","Mod","1","B","Choke = On\n","");CHECK(parse(s,&kit),"ratio preset refused: %s",kit.message);free(s);
 CHECK(field(&kit,0,8)==75,"ratio mix %g",field(&kit,0,8));CHECK(field(&kit,0,14)==2&&kit.clap==1,"clap");
 CHECK(kit.outputs==1&&field(&kit,0,23)==1,"output/choke");CHECK(field(&kit,0,19)==100&&fabsf(field(&kit,1,19)-50.1187234f)<1e-4f,"attenuation %g",field(&kit,1,19));
 /* -inf level and an extreme resonance (1.1.2: ratio of the Q range, not a clamp). */
 s=preset("-inf","50","Exp","5581.6","A","","");CHECK(parse(s,&kit),"inf preset refused: %s",kit.message);free(s);
 CHECK(field(&kit,0,19)==0&&fabsf(field(&kit,0,11)-11.2072961f)<1e-5f&&kit.clamped==0,"ratio/inf %.9g %.9g %d",field(&kit,0,19),field(&kit,0,11),kit.clamped);
 s=preset("0","50","Exp","1","A","","");char *r=strstr(s,"ModRate = 100");memcpy(r,"ModRate = inf",13);CHECK(parse(s,&kit)&&field(&kit,0,6)==20000,"ModRate inf");free(s);
 /* Missing optional fields take the FM-1 defaults. */
 s=preset("0","50","Exp","1","A","","");
 for(const char *k[]={"Pan = -10\n","NVel = 50\n","OscVel = 40\n","DistAmt = 20\n","EQGain = 3\n","EQFreq = 500\n","NStereo = On\n","ModVel = 10\n"},**i=k;i<k+8;i++){char *at=strstr(s,*i);memmove(at,at+strlen(*i),strlen(at+strlen(*i))+1);}
 CHECK(parse(s,&kit),"defaults refused: %s",kit.message);free(s);
 CHECK(field(&kit,0,22)==0&&field(&kit,0,21)==100&&field(&kit,0,20)==100&&field(&kit,0,16)==0&&field(&kit,0,18)==0&&field(&kit,0,17)==1000&&field(&kit,0,15)==0&&field(&kit,0,7)==0,"defaults");
 /* BOM, CRLF, '=' after the header and commas. */
 s=preset("0","50","Exp","1","A","","");size_t n=strlen(s);char *crlf=malloc(2*n+4);size_t m=3;memcpy(crlf,"\xEF\xBB\xBF",3);
 for(size_t i=0;i<n;i++){if(s[i]=='\n')crlf[m++]='\r';crlf[m++]=s[i];}crlf[m]=0;CHECK(parse(crlf,&kit),"BOM/CRLF refused: %s",kit.message);free(crlf);free(s);
 /* Errors. */
 expect_error("","Not a MicroTonic");
 expect_error("MicroTonicPresetV2: { }","Not a MicroTonic");
 expect_error("MicroTonicPresetV3: { Tempo = 120","Incomplete");
 expect_error("MicroTonicPresetV3: { DrumPatches: { } }","Sound 1 missing");
 s=preset("0","50","Exp","1","A","","");strstr(s,"\t\t8: {")[2]='9';expect_error(s,"Sound 8 missing");free(s);
 s=preset("0","50","Exp","1","A","","extra");expect_error(s,"Unexpected content");free(s);
 s=preset("0","50","Exp","1","A","Pan = 3\n","");expect_error(s,"Repeated field");free(s);
 s=preset("0","50","Wobble","1","A","","");expect_error(s,"Sound 1: unknown NEnvMod");free(s);
 s=preset("0","50","Exp","abc","A","","");expect_error(s,"Sound 1: invalid NFilQ");free(s);
 s=preset("0","50","Exp","nan","A","","");expect_error(s,"Sound 1: invalid NFilQ");free(s);
 s=preset("inf","50","Exp","1","A","","");expect_error(s,"Sound 1: invalid Level");free(s);
 s=preset("0","0/0","Exp","1","A","","");expect_error(s,"Sound 1: invalid Mix");free(s);
 s=preset("0","1/2/3","Exp","1","A","","");expect_error(s,"Sound 1: invalid Mix");free(s);
 s=preset("0","50","Exp","1","A","","");r=strstr(s,"OscFreq = 60\n");memset(r,' ',13);expect_error(s,"Sound 1: missing OscFreq");free(s);
 s=preset("0","50","Exp","1","A","","");r=strstr(s,"\"Snd\"");r[4]=' ';expect_error(s,"Unclosed text");free(s);
 s=preset("0","50","Exp","1","A","","");s[40]=0;CHECK(!mt_parse(s,strlen(s)+1+10,&kit)&&strstr(kit.message,"Not a text"),"NUL accepted: %s",kit.message);free(s);
 /* Bounds: depth, token count, size. */
 {char deep[400]="MicroTonicPresetV3: ";for(int i=0;i<20;i++)strcat(deep,"{ a: ");strcat(deep,"1");for(int i=0;i<20;i++)strcat(deep," }");expect_error(deep,"too complex");}
 {size_t cap=MT_MAX_TOKENS*4+64;char *t=malloc(cap);strcpy(t,"MicroTonicPresetV3: { L: {");size_t k=strlen(t);for(int i=0;i<MT_MAX_TOKENS;i++){t[k++]=' ';t[k++]='x';}strcpy(t+k," } }");expect_error(t,"too complex");free(t);}
 {char *big=malloc(MT_MAX_BYTES+2);memset(big,' ',MT_MAX_BYTES+1);big[MT_MAX_BYTES+1]=0;expect_error(big,"too large");free(big);}
 printf("%s mtpreset unit tests (%d failure%s)\n",failures?"FAIL":"PASS",failures,failures==1?"":"s");
}

static int dump(const char *path){
 MTKit kit;if(!mt_load(path,&kit)){printf("ERR %s\n",kit.message);return 0;}
 printf("OK %d %d %d %.9g\n",kit.clamped,kit.clap,kit.outputs,kit.attenuation_db);
 for(int v=0;v<8;v++){for(int f=0;f<PT_FIELDS;f++)printf("%s%.9g",f?" ":"",kit.values[v][f]);printf("\n");}
 return 0;
}

int main(int argc,char **argv){
 if(argc==3&&!strcmp(argv[1],"--dump"))return dump(argv[2]);
 unit_tests();return failures!=0;
}
