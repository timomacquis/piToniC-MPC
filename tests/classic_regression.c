/* πToniC 1.1.0 : empreinte du son CLASSIC (reverb + delay actifs, 8 voix).
 * Doit rester identique à celle de la 1.0.4 : le mode CLASSIC ne change aucun son existant. */
#include <assert.h>
#include "../src/engine.c"
static uint64_t h=1469598103934665603ULL;
static void eat(const int16_t*o,int n){for(int i=0;i<n*2;i++){h^=(uint16_t)o[i];h*=1099511628211ULL;}}
int main(int argc,char**argv){
 PT*p=create(0);assert(p);int16_t out[256];
 const char *setup[][2]={{"rev_size","80"},{"rev_decay","4.5"},{"rev_damp","30"},{"rev_return","70"},{"del_sync","1"},{"del_div","3"},
  {"del_feedback","60"},{"del_tone","40"},{"del_return","65"},{"lfo_bpm","118"},{"send0_reverb","40"},{"send1_reverb","90"},
  {"send2_delay","75"},{"send3_reverb","55"},{"send3_delay","55"},{"send5_delay","100"},{"send6_reverb","100"},{"v6_decay","800"}};
 for(unsigned i=0;i<sizeof setup/sizeof*setup;i++)set(p,setup[i][0],setup[i][1]);
 if(argc>1&&!strcmp(argv[1],"classic"))set(p,"fxn_mode","0");
 for(int b=0;b<1723;b++){ /* ~5 s */
  if(b%43==0)trigger(p,(b/43)%8,90+(b/43)%30);
  if(b%86==21)trigger(p,0,127);
  render(p,out,128);eat(out,128);}
 printf("%016llx\n",(unsigned long long)h);destroy(p);return 0;
}
