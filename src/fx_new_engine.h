/* πToniC 1.1.0 — paramètres des effets NEW côté moteur (inclus par engine.c après la structure PT). */
static const char *const fxn_keys[FXN_COUNT]={"fxn_rtype","fxn_rdecay","fxn_rlive","fxn_rwarmth","fxn_dsync","fxn_ddiv","fxn_dms","fxn_dfeedback","fxn_dfilter"};
static const float fxn_lo[FXN_COUNT]={0,0,0,0,0,0,10,0,-100},fxn_hi[FXN_COUNT]={9,100,100,100,1,6,2000,120,100},fxn_defaults[FXN_COUNT]={1,55,60,35,1,3,375,45,0};
static const int fxn_int[FXN_COUNT]={1,0,0,0,1,1,0,0,0};
static const char *const fxn_types[]={"ROOM","HALL","SPRING","PLATE","MODULATED","REVERSE","HALO","HALO+","ABYSS","CASCADE"};
static const char *const fxn_divs[]={"1/16","1/8","1/8 D","1/4","1/4 D","1/2","1"};
/* Q-Links de la page FX : 8 réglages selon le mode affiché. */
static const char *const fxq_classic[8]={"rev_size","rev_decay","rev_damp","rev_return","fx_time","del_feedback","del_tone","del_return"};
static const char *const fxq_new[8]={"fxn_rdecay","fxn_rlive","fxn_rwarmth","rev_return","fxn_time","fxn_dfeedback","fxn_dfilter","del_return"};
enum {FXND_MODE=FXN_COUNT,FXND_TIME,FXND_Q};
static void set(void *ptr,const char *key,const char *val);
static int get(void *ptr,const char *key,char *buf,int len);
static void fxn_mark_q(PT *p){for(int i=0;i<8;i++)p->fxn_dirty[FXND_Q+i]=1;}
static void fxn_apply_all(PT *p,int mode,const float *v){
 if(mode!=p->fx_new&&p->fxn)fxn_reset(p->fxn);p->fx_new=mode;
 for(int i=0;i<FXN_COUNT;i++){p->fxn_p[i]=v[i];fxn_set(p->fxn,i,v[i]);}
 p->fxn_g[0]=p->fxn_g[1]=0;memset(p->fxn_dirty,1,sizeof p->fxn_dirty);}
static int fxn_index(const char *key){for(int i=0;i<FXN_COUNT;i++)if(!strcmp(key,fxn_keys[i]))return i;return -1;}
static int fxn_set_key(PT *p,const char *key,float x){
 if(strncmp(key,"fxn_",4)&&strncmp(key,"fxq",3))return 0;
 if(!strcmp(key,"fxn_mode")){int m=x>.5f;if(m!=p->fx_new){if(p->fxn)fxn_reset(p->fxn);p->fx_new=m;p->fxn_g[0]=p->fxn_g[1]=0;p->fxn_dirty[FXND_MODE]=1;fxn_mark_q(p);}return 1;}
 if(!strcmp(key,"fxn_time")){x=clamp(x,0,1);int sync=p->fxn_p[FXN_DSYNC]>.5f;return fxn_set_key(p,sync?"fxn_ddiv":"fxn_dms",sync?roundf(x*6):10+1990*x*x);}
 int q;if(sscanf(key,"fxq%d",&q)==1&&q>=0&&q<8&&!key[4]){x=clamp(x,0,1);const char *t=(p->fx_new?fxq_new:fxq_classic)[q];char v[32];
  if(!strcmp(t,"fx_time")||!strcmp(t,"fxn_time"))snprintf(v,sizeof v,"%.9g",x);
  else{int i=fxn_index(t);float lo,hi;if(i>=0){lo=fxn_lo[i];hi=fxn_hi[i];}else{int j=0;while(j<FX_COUNT&&strcmp(t,fx_keys[j]))j++;if(j==FX_COUNT)return 1;lo=fx_lo[j];hi=fx_hi[j];}snprintf(v,sizeof v,"%.9g",lo+(hi-lo)*x);}
  set(p,t,v);p->fxn_dirty[FXND_Q+q]=1;return 1;}
 int i=fxn_index(key);if(i<0)return 1;x=clamp(x,fxn_lo[i],fxn_hi[i]);if(fxn_int[i])x=roundf(x);
 if(x!=p->fxn_p[i]){p->fxn_p[i]=x;fxn_set(p->fxn,i,x);p->fxn_dirty[i]=1;if(i==FXN_DSYNC||i==FXN_DDIV||i==FXN_DMS)p->fxn_dirty[FXND_TIME]=1;fxn_mark_q(p);}return 1;}
static int fxn_display(PT *p,int i,char *buf,int len){float x=p->fxn_p[i];
 switch(i){case FXN_RTYPE:return snprintf(buf,len,"%s",fxn_types[(int)x]);case FXN_DSYNC:return snprintf(buf,len,"%s",x>.5f?"SYNC":"FREE");case FXN_DDIV:return snprintf(buf,len,"%s",fxn_divs[(int)x]);
 case FXN_DMS:return snprintf(buf,len,"%.0f ms",x);case FXN_DFILTER:return x<-.5f?snprintf(buf,len,"LP %.0f",-x):x>.5f?snprintf(buf,len,"HP %.0f",x):snprintf(buf,len,"OFF");default:return snprintf(buf,len,"%.0f %%",x);}}
/* Retourne -1 si la clé n'est pas un paramètre NEW. */
static int fxn_get_key(PT *p,const char *key,char *buf,int len){
 if(strncmp(key,"fxn_",4)&&strncmp(key,"fxq",3))return -1;
 char bare[40];size_t k=strlen(key);if(k>=sizeof bare)return -1;strcpy(bare,key);int display=k>8&&!strcmp(bare+k-8,"_display"),name=k>5&&!strcmp(bare+k-5,"_name");if(display)bare[k-8]=0;if(name)bare[k-5]=0;
 if(!strcmp(bare,"fxn_mode"))return display?snprintf(buf,len,"%s",p->fx_new?"ON":"OFF"):name?snprintf(buf,len,"EXTENDED"):snprintf(buf,len,"%d",p->fx_new);
 if(!strcmp(bare,"fxn_time")){int sync=p->fxn_p[FXN_DSYNC]>.5f;if(display)return fxn_display(p,sync?FXN_DDIV:FXN_DMS,buf,len);if(name)return snprintf(buf,len,"TIME");
  return snprintf(buf,len,"%.9g",sync?p->fxn_p[FXN_DDIV]/6:sqrtf((p->fxn_p[FXN_DMS]-10)/1990));}
 int q;if(sscanf(bare,"fxq%d",&q)==1&&q>=0&&q<8&&!bare[4]){const char *t=(p->fx_new?fxq_new:fxq_classic)[q];char sub[48];
  if(name){static const char *const nc[8]={"SIZE","DECAY","DAMP","REVERB","TIME","FEEDBACK","TONE","DELAY"},*const nn[8]={"DECAY","LIVELINESS","WARMTH","REVERB","TIME","FEEDBACK","FILTER","DELAY"};return snprintf(buf,len,"%s",(p->fx_new?nn:nc)[q]);}
  if(display){snprintf(sub,sizeof sub,"%s_display",t);return get(p,sub,buf,len);}
  if(!strcmp(t,"fx_time")||!strcmp(t,"fxn_time"))return get(p,t,buf,len);
  char v[32];if(get(p,t,v,sizeof v)<=0)return snprintf(buf,len,"0");float x=strtof(v,NULL),lo,hi;int i=fxn_index(t);
  if(i>=0){lo=fxn_lo[i];hi=fxn_hi[i];}else{int j=0;while(j<FX_COUNT&&strcmp(t,fx_keys[j]))j++;if(j==FX_COUNT)return snprintf(buf,len,"0");lo=fx_lo[j];hi=fx_hi[j];}
  return snprintf(buf,len,"%.9g",(x-lo)/(hi-lo));}
 int i=fxn_index(bare);if(i<0)return -1;if(display)return fxn_display(p,i,buf,len);if(name)return -1;return snprintf(buf,len,"%.9g",p->fxn_p[i]);}
static int fxn_next_dirty(PT *p,char *key,int len){for(int i=0;i<(int)sizeof p->fxn_dirty;i++)if(p->fxn_dirty[i]){p->fxn_dirty[i]=0;
 if(i<FXN_COUNT)snprintf(key,len,"%s",fxn_keys[i]);else if(i==FXND_MODE)snprintf(key,len,"fxn_mode");else if(i==FXND_TIME)snprintf(key,len,"fxn_time");else snprintf(key,len,"fxq%d",i-FXND_Q);return 1;}return 0;}
