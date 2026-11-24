/* πToniC MicroTonicPresetV3 kit reader. Original implementation, MIT.
 *
 * Kit only: the eight DrumPatches are converted with the rules used for the
 * 150 BEST kits (scripts/convert_microtonic_private.py), plus the tolerance
 * of the FM-1 importer for options that Patternarium files leave out (Choke,
 * NStereo, velocities, drive, EQ, pan). Patterns, tempo, swing and mutes are
 * syntax-checked, then ignored: the Force sequences the kit itself.
 *
 * Bounded data-only parser, no evaluation: 1 MiB, 20000 tokens, depth 16,
 * 10000 values, 128 fields per group. mt_load() opens and allocates, so it is
 * called from the sample worker only, never from the audio thread.
 * The output is written only when the whole file is valid. */
#pragma once
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include "fields.h"

#define MT_MAX_BYTES (1024u*1024u)
#define MT_MAX_TOKENS 20000
#define MT_MAX_DEPTH 16
#define MT_MAX_NODES 10000
#define MT_MAX_KEYS 128
#define MT_MESSAGE 96

typedef struct {
 float values[8][PT_FIELDS]; /* both A and B banks receive these */
 int clamped;                /* values brought into πToniC ranges */
 int clap;                   /* NEnvMod = Mod, played as CLAP */
 int outputs;                /* Output other than A, sent to the stereo mix */
 float attenuation_db;       /* common level reduction for positive gains */
 char message[MT_MESSAGE];   /* error text when mt_load/mt_parse return 0 */
} MTKit;

/* Same order as pt_fields (FREQ ... CHOKE). */
static const char *const mt_keys[PT_FIELDS]={"OscFreq","OscWave","OscAtk","OscDcy","ModMode","ModAmt","ModRate","ModVel","Mix","NFilMod","NFilFrq","NFilQ","NEnvAtk","NEnvDcy","NEnvMod","NStereo","DistAmt","EQFreq","EQGain","Level","OscVel","NVel","Pan","Choke"};
/* Defaults when a field is absent (FM-1 importer 0.9.1); NULL = required. */
static const char *const mt_defaults[PT_FIELDS]={[7]="0",[15]="Off",[16]="0",[17]="1000",[18]="0",[20]="100",[21]="100",[22]="0",[23]="Off"};
enum {MT_ROOT=1,MT_PATCHES,MT_SOUND};

typedef struct {int at,len;} MTToken;
typedef struct {
 const char *text;MTToken *tok;int count,i,nodes;
 int field[8][PT_FIELDS],output[8],sound[8]; /* token index + 1, 0 = absent */
 char message[MT_MESSAGE];
} MTParser;

static int mt_fail(MTParser *p,const char *message){if(!p->message[0])snprintf(p->message,sizeof p->message,"%s",message);return 0;}
static int mt_space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\f'||c=='\v';}
static int mt_punct(char c){return c=='{'||c=='}'||c=='='||c==':'||c==',';}
static int mt_is(const MTParser *p,int i,const char *s){size_t n=strlen(s);return i<p->count&&(size_t)p->tok[i].len==n&&!memcmp(p->text+p->tok[i].at,s,n);}
static int mt_is_punct(const MTParser *p,int i){return i<p->count&&p->tok[i].len==1&&mt_punct(p->text[p->tok[i].at]);}
/* Token content without surrounding quotes (escapes are kept as written). */
static const char *mt_content(const MTParser *p,int i,int *len){const char *s=p->text+p->tok[i].at;int n=p->tok[i].len;if(n>=2&&s[0]=='"'){s++;n-=2;}*len=n;return s;}
static int mt_equal(const MTParser *p,int a,int b){int na,nb;const char *x=mt_content(p,a,&na),*y=mt_content(p,b,&nb);return na==nb&&!memcmp(x,y,(size_t)na);}
static int mt_named(const MTParser *p,int i,const char *s){int n;const char *x=mt_content(p,i,&n);return (size_t)n==strlen(s)&&!memcmp(x,s,(size_t)n);}

static int mt_tokenize(MTParser *p,size_t n){
 const char *t=p->text;size_t at=0;
 if(n>=3&&!memcmp(t,"\xEF\xBB\xBF",3))at=3;
 while(at<n){
  char c=t[at];if(mt_space(c)){at++;continue;}
  if(!c)return mt_fail(p,"Not a text file");
  size_t start=at;
  if(mt_punct(c))at++;
  else if(c=='"'){
   for(at++;;){
    if(at>=n||t[at]=='\r'||t[at]=='\n'||!t[at])return mt_fail(p,"Unclosed text");
    if(t[at]=='"'){at++;break;}
    if(t[at]!='\\'){at++;continue;}
    if(at+1>=n)return mt_fail(p,"Unclosed text");
    char e=t[at+1];
    if(e=='u'){if(at+5>=n)return mt_fail(p,"Invalid text escape");for(int k=2;k<6;k++)if(!isxdigit((unsigned char)t[at+k]))return mt_fail(p,"Invalid text escape");at+=6;}
    else if(e&&strchr("\"\\/bfnrt",e))at+=2;
    else return mt_fail(p,"Invalid text escape");
   }
  }
  else while(at<n&&t[at]&&!mt_space(t[at])&&!mt_punct(t[at])&&t[at]!='"')at++;
  if(p->count>=MT_MAX_TOKENS)return mt_fail(p,"Preset too complex");
  p->tok[p->count].at=(int)start;p->tok[p->count].len=(int)(at-start);p->count++;
 }
 return 1;
}

static int mt_value(MTParser *p,int depth,int role,int sound){
 if(++p->nodes>MT_MAX_NODES||depth>MT_MAX_DEPTH)return mt_fail(p,"Preset too complex");
 if(p->i>=p->count)return mt_fail(p,"Incomplete preset");
 if(!mt_is(p,p->i,"{")){if(mt_is_punct(p,p->i))return mt_fail(p,"Missing value");p->i++;return 1;}
 p->i++;
 const int object=mt_is(p,p->i+1,":")||mt_is(p,p->i+1,"=");
 int keys[MT_MAX_KEYS],nk=0;
 for(;;){
  if(p->i>=p->count)return mt_fail(p,"Incomplete preset");
  if(mt_is(p,p->i,"}"))break;
  if(object){
   const int key=p->i++;
   if(mt_is_punct(p,key)||!(mt_is(p,p->i,":")||mt_is(p,p->i,"=")))return mt_fail(p,"Invalid field");
   p->i++;
   for(int k=0;k<nk;k++)if(mt_equal(p,keys[k],key))return mt_fail(p,"Repeated field");
   if(nk==MT_MAX_KEYS)return mt_fail(p,"Preset too complex");
   keys[nk++]=key;
   int child=0,child_sound=-1,len;const char *name=mt_content(p,key,&len);
   if(role==MT_ROOT&&mt_named(p,key,"DrumPatches"))child=MT_PATCHES;
   else if(role==MT_PATCHES&&len==1&&name[0]>='1'&&name[0]<='8'&&mt_is(p,p->i,"{")){child=MT_SOUND;child_sound=name[0]-'1';p->sound[child_sound]=1;}
   else if(role==MT_SOUND&&!mt_is(p,p->i,"{")){
    for(int f=0;f<PT_FIELDS;f++)if(mt_named(p,key,mt_keys[f])){p->field[sound][f]=p->i+1;break;}
    if(mt_named(p,key,"Output"))p->output[sound]=p->i+1;
   }
   if(!mt_value(p,depth+1,child,child_sound))return 0;
  }
  else if(!mt_value(p,depth+1,0,-1))return 0;
  if(mt_is(p,p->i,","))p->i++;
 }
 p->i++;return 1;
}

/* Plain decimal numbers as written by MicroTonic, plus inf / +inf / -inf. */
static int mt_number(const char *s,int n,double *out){
 char b[64];if(n<=0||n>=(int)sizeof b)return 0;memcpy(b,s,(size_t)n);b[n]=0;
 int i=b[0]=='+'||b[0]=='-';
 if(!strcasecmp(b+i,"inf")){*out=b[0]=='-'?-INFINITY:INFINITY;return 1;}
 int digits=0;while(isdigit((unsigned char)b[i])){i++;digits++;}
 if(b[i]=='.'){i++;while(isdigit((unsigned char)b[i])){i++;digits++;}}
 if(!digits)return 0;
 if(b[i]=='e'||b[i]=='E'){i++;if(b[i]=='+'||b[i]=='-')i++;if(!isdigit((unsigned char)b[i]))return 0;while(isdigit((unsigned char)b[i]))i++;}
 if(b[i])return 0;
 char *end;double x=strtod(b,&end);if(*end||isnan(x))return 0;*out=x;return 1;
}
static int mt_option(const char *s,int n,const char *const *list,int count){for(int i=0;i<count;i++)if((size_t)n==strlen(list[i])&&!memcmp(s,list[i],(size_t)n))return i;return -1;}

/* The value text of one field, or its default; NULL when a required field is absent. */
static const char *mt_text(const MTParser *p,int v,int f,int *len){
 int t=p->field[v][f];if(t)return mt_content(p,t-1,len);
 if(!mt_defaults[f])return NULL;
 *len=(int)strlen(mt_defaults[f]);return mt_defaults[f];
}
static int mt_bad(MTParser *p,int v,int f,const char *what){char m[MT_MESSAGE];snprintf(m,sizeof m,"Sound %d: %s %s",v+1,what,mt_keys[f]);return mt_fail(p,m);}

#define MT_Q_LO .1
#define MT_Q_HI 10000.
static int mt_convert(MTParser *p,MTKit *kit){
 static const char *const waves[]={"Sine","Triangle","Saw"},*const modes[]={"Decay","Sine","Noise"},*const filters[]={"LP","BP","HP"},*const envs[]={"Exp","Linear","Mod"},*const offon[]={"Off","On"};
 static const double wave_value[]={0,100./3,200./3};
 double level[8],attenuation=0;
 for(int v=0;v<8;v++){
  char m[MT_MESSAGE];if(!p->sound[v]){snprintf(m,sizeof m,"Sound %d missing",v+1);return mt_fail(p,m);}
  int n;const char *s=mt_text(p,v,19,&n);
  if(!s)return mt_bad(p,v,19,"missing");
  if(!mt_number(s,n,&level[v])||level[v]==INFINITY)return mt_bad(p,v,19,"invalid");
  if(level[v]>attenuation)attenuation=level[v];
 }
 memset(kit,0,sizeof *kit);kit->attenuation_db=(float)attenuation;
 for(int v=0;v<8;v++){
  for(int f=0;f<PT_FIELDS;f++){
   int n,o=0;double x;const char *s=mt_text(p,v,f,&n);
   if(!s)return mt_bad(p,v,f,"missing");
   switch(f){
    case 1:o=mt_option(s,n,waves,3);x=o<0?0:wave_value[o];break;
    case 4:o=mt_option(s,n,modes,3);x=o;break;
    case 9:o=mt_option(s,n,filters,3);x=o;break;
    case 14:o=mt_option(s,n,envs,3);x=o;if(o==2)kit->clap++;break;
    case 15:case 23:o=mt_option(s,n,offon,2);x=o;break;
    case 8:{ /* Mix: "osc/noise" levels, or the oscillator share 0..100 */
     const char *slash=memchr(s,'/',(size_t)n);double a,b;
     if(slash){if(!mt_number(s,(int)(slash-s),&a)||!mt_number(slash+1,n-(int)(slash-s)-1,&b)||!isfinite(a)||!isfinite(b)||a<0||b<0||a+b<=0)return mt_bad(p,v,f,"invalid");x=100*b/(a+b);}
     else{if(!mt_number(s,n,&a))return mt_bad(p,v,f,"invalid");x=100-a;}
     break;}
    case 16:if(!mt_number(s,n,&x))return mt_bad(p,v,f,"invalid");x*=.5;break; /* 0..100 -> DRIVE dB */
    case 11:{ /* 1.1.2: Q 0.1..10000 placed at the same ratio of RESONANCE 0.1..20 */
     if(!mt_number(s,n,&x))return mt_bad(p,v,f,"invalid");
     double q=x<MT_Q_LO?MT_Q_LO:x>MT_Q_HI?MT_Q_HI:x;if(q!=x)kit->clamped++;
     /* bounds read back as decimals (fields.h stores floats) to match the converter */
     char b[32];snprintf(b,sizeof b,"%.7g",pt_fields[f].lo);double lo=strtod(b,NULL);snprintf(b,sizeof b,"%.7g",pt_fields[f].hi);double hi=strtod(b,NULL);
     x=lo+(q-MT_Q_LO)/(MT_Q_HI-MT_Q_LO)*(hi-lo);break;}
    case 19:x=100*pow(10,(level[v]-attenuation)/20);break;
    default:if(!mt_number(s,n,&x))return mt_bad(p,v,f,"invalid");
   }
   if(o<0)return mt_bad(p,v,f,"unknown");
   const PTField *d=&pt_fields[f];double y=x<d->lo?d->lo:x>d->hi?d->hi:x;
   if(y!=x)kit->clamped++;
   /* Same decimal path as the converter and the saved state, so an imported
    * kit is bit-identical to the matching converted .xpl preset. */
   char text[32];snprintf(text,sizeof text,"%.9g",y);kit->values[v][f]=strtof(text,NULL);
  }
  if(p->output[v]){int n;const char *s=mt_content(p,p->output[v]-1,&n);if(!(n==1&&s[0]=='A'))kit->outputs++;}
 }
 return 1;
}

/* Parse a whole .mtpreset text of n bytes. On failure kit is left unchanged
 * except kit->message. */
static int mt_parse(const char *text,size_t n,MTKit *kit){
 MTParser *p=calloc(1,sizeof *p);MTKit *out=calloc(1,sizeof *out);int ok=0;
 if(!p||!out){if(kit)snprintf(kit->message,sizeof kit->message,"Out of memory");free(p);free(out);return 0;}
 p->text=text;
 if(n>MT_MAX_BYTES)mt_fail(p,"File too large (1 MB max)");
 else if(!(p->tok=malloc(MT_MAX_TOKENS*sizeof *p->tok)))mt_fail(p,"Out of memory");
 else if(mt_tokenize(p,n)){
  if(!mt_is(p,0,"MicroTonicPresetV3")||!(mt_is(p,1,":")||mt_is(p,1,"=")))mt_fail(p,"Not a MicroTonic V3 preset");
  else{p->i=2;if(mt_value(p,0,MT_ROOT,-1)){if(p->i!=p->count)mt_fail(p,"Unexpected content after preset");else ok=mt_convert(p,out);}}
 }
 if(ok){*kit=*out;kit->message[0]=0;}
 else snprintf(kit->message,sizeof kit->message,"%s",p->message[0]?p->message:"Invalid preset");
 free(p->tok);free(p);free(out);return ok;
}

/* Worker only: reads one regular file of at most 1 MiB. */
static int mt_load(const char *path,MTKit *kit){
 FILE *f=fopen(path,"rb");struct stat st;
 if(!f){snprintf(kit->message,sizeof kit->message,"Missing / unreadable file");return 0;}
 if(fstat(fileno(f),&st)||!S_ISREG(st.st_mode)){fclose(f);snprintf(kit->message,sizeof kit->message,"Not a file");return 0;}
 if(st.st_size>(off_t)MT_MAX_BYTES){fclose(f);snprintf(kit->message,sizeof kit->message,"File too large (1 MB max)");return 0;}
 size_t n=(size_t)st.st_size;char *text=malloc(n+1);
 if(!text){fclose(f);snprintf(kit->message,sizeof kit->message,"Out of memory");return 0;}
 size_t got=fread(text,1,n,f);fclose(f);
 if(got!=n){free(text);snprintf(kit->message,sizeof kit->message,"Missing / unreadable file");return 0;}
 text[n]=0;int ok=mt_parse(text,n,kit);free(text);return ok;
}
