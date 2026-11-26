/* πToniC 1.2.0 : source WAVETABLE, 8 tables de 16 positions, écrites par formule (aucune table externe).
 * Chaque position est calculée une fois par processus, au premier create(), en 9 versions limitées en
 * harmoniques (FFT 512 points) ; l'oscillateur lit la version adaptée à sa hauteur, sans repliement.
 * Mémoire statique partagée par toutes les instances : 8 x 16 x 9 x 513 floats = 2,4 Mo.
 * Les mêmes formules dessinent les bandes d'images du skin (scripts/build_skin.py, wt_naive). */
#pragma once
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#define WT_TABLES 8
#define WT_FRAMES 16
#define WT_SIZE 512
#define WT_LEVELS 9
enum {WT_PWM,WT_HARMONICS,WT_FORMANT,WT_SYNC,WT_FOLD,WT_CRUSH,WT_FM,WT_NOISE};
static float wt_data[WT_TABLES][WT_FRAMES][WT_LEVELS][WT_SIZE+1];
static pthread_once_t wt_once=PTHREAD_ONCE_INIT;
#define WT_PI 3.14159265358979323846
/* Bruit fixe de NOISE : xorshift32, graine 0x9E3779B9, une valeur par point (identique côté Python). */
static double wt_noise(int i){uint32_t x=0x9E3779B9u;for(int k=0;k<=i;k++){x^=x<<13;x^=x>>17;x^=x<<5;}return (double)(x>>8)*(2.0/16777216.0)-1.0;}
/* Forme naïve de la table t à la position k (0..1), point t01 (0..1). */
static double wt_naive(int t,double k,double x,int i){
 switch(t){
  case WT_PWM:return x<.5-.46*k?1:-1;
  case WT_HARMONICS:{double n=1+15*k,s=0;for(int h=1;h<=16;h++){double g=n-(h-1);g=g<0?0:g>1?1:g;s+=g*sin(2*WT_PI*h*x)/h;}return s;}
  case WT_FORMANT:{double f=3+6*k;return sin(2*WT_PI*x*f)*(.5-.5*cos(2*WT_PI*x));}
  case WT_SYNC:{double ph=x*(1+6*k);ph-=floor(ph);return 1-2*ph;}
  case WT_FOLD:{double g=1+5*k;return sin(WT_PI/2*g*sin(2*WT_PI*x));}
  case WT_CRUSH:{double lv=round(16-14*k);if(lv<2)lv=2;return round(sin(2*WT_PI*x)*lv/2)/(lv/2);}
  case WT_FM:return sin(2*WT_PI*x+5*k*sin(2*WT_PI*3*x));
  default:{double sq=x<.5?1:-1;return (1-k)*sq+k*wt_noise(i);}
 }
}
static void wt_fft(double *re,double *im,int n,int inverse){
 for(int i=1,j=0;i<n;i++){int bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j){double t=re[i];re[i]=re[j];re[j]=t;t=im[i];im[i]=im[j];im[j]=t;}}
 for(int len=2;len<=n;len<<=1){double a=2*WT_PI/len*(inverse?1:-1),wr=cos(a),wi=sin(a);
  for(int i=0;i<n;i+=len){double cr=1,ci=0;for(int j=0;j<len/2;j++){int u=i+j,v=u+len/2;double tr=re[v]*cr-im[v]*ci,ti=re[v]*ci+im[v]*cr;re[v]=re[u]-tr;im[v]=im[u]-ti;re[u]+=tr;im[u]+=ti;double nr=cr*wr-ci*wi;ci=cr*wi+ci*wr;cr=nr;}}}
}
static void wt_build(void){
 static double sr[WT_SIZE],si[WT_SIZE],re[WT_SIZE],im[WT_SIZE];
 for(int t=0;t<WT_TABLES;t++){double peak=0;
  for(int f=0;f<WT_FRAMES;f++){double k=(double)f/(WT_FRAMES-1);
   for(int i=0;i<WT_SIZE;i++){sr[i]=wt_naive(t,k,(double)i/WT_SIZE,i);si[i]=0;}
   wt_fft(sr,si,WT_SIZE,0);
   for(int l=0;l<WT_LEVELS;l++){int top=(WT_SIZE/2)>>l;if(top>=WT_SIZE/2)top=WT_SIZE/2-1;
    /* DC retiré ; harmoniques 1..top gardées (spectre symétrique). */
    for(int i=0;i<WT_SIZE;i++){int h=i<=WT_SIZE/2?i:WT_SIZE-i;int keep=h>=1&&h<=top;re[i]=keep?sr[i]:0;im[i]=keep?si[i]:0;}
    wt_fft(re,im,WT_SIZE,1);float *d=wt_data[t][f][l];
    for(int i=0;i<WT_SIZE;i++){d[i]=(float)(re[i]/WT_SIZE);if(fabs(d[i])>peak)peak=fabs(d[i]);}
    d[WT_SIZE]=d[0];}}
  /* Même gain pour toute la table : crête 0,95 sur l'ensemble des positions et des versions, proche de l'oscillateur SYNTH. */
  float g=(float)(peak>1e-9?.95/peak:1);for(int f=0;f<WT_FRAMES;f++)for(int l=0;l<WT_LEVELS;l++)for(int i=0;i<=WT_SIZE;i++)wt_data[t][f][l][i]*=g;}
}
static void wt_init(void){pthread_once(&wt_once,wt_build);}
/* Version adaptée à l'incrément de phase dt : harmonique la plus haute <= Nyquist. */
static inline int wt_level(float dt){float x=dt*WT_SIZE;if(x<=1)return 0;int l=(int)ceilf(log2f(x));return l<0?0:l>=WT_LEVELS?WT_LEVELS-1:l;}
/* Lecture : table, position 0..1, phase 0..1, version l. Interpolation linéaire entre points et entre positions. */
static inline float wt_read(int t,float pos,float phase,int l){
 float fp=(pos<0?0:pos>1?1:pos)*(WT_FRAMES-1);int f0=(int)fp;if(f0>WT_FRAMES-2)f0=WT_FRAMES-2;float ff=fp-f0;
 float x=phase*WT_SIZE;int i=(int)x;float xf=x-i;i&=WT_SIZE-1;
 const float *a=wt_data[t][f0][l],*b=wt_data[t][f0+1][l];float va=a[i]+(a[i+1]-a[i])*xf,vb=b[i]+(b[i+1]-b[i])*xf;return va+(vb-va)*ff;}
/* Balayage : s en unités de durée (1 = SCAN TIME). ONE s'arrête sur END, LOOP recommence, PING-PONG fait l'aller-retour. */
static inline float wt_scan_shape(int mode,float s){if(mode==0)return s>1?1:s;if(mode==1)return s-floorf(s);float u=fmodf(s,2.f);return u>1?2-u:u;}
