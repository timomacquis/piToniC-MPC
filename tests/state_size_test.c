/* πToniC 1.0.4 : taille de l'état projet face au tampon du wrapper (MPC_CHUNK_BYTES = 8192). */
#include "engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(void){const mpc_engine_t *e=mpc_engine();void *p=e->create(".");static char buf[65536];char path[512],key[32];
 int lens[]={60,120,180,250,400};
 for(int t=0;t<5;t++){
  for(int v=0;v<8;v++){char sel[16];snprintf(sel,sizeof sel,"%d",v);e->set_param(p,"selected",sel);
   int n=snprintf(path,sizeof path,"/media/ForceHD/Samples/Voice%d/",v);while(n<lens[t]-4)path[n++]='x';strcpy(path+n,".wav");
   e->set_param(p,"sample_path",path);snprintf(key,sizeof key,"v%d_source",v);e->set_param(p,key,"2"); /* 1.2.0 : SAMPLE = 2 */
   e->set_param(p,"random","1");e->set_param(p,"compare","1");e->set_param(p,"random","1");}
  int used=e->get_param(p,"state",buf,sizeof buf);int fit=e->get_param(p,"state",buf,8192);
  printf("chemins de %3d car. x 8 : état %5d octets, tient dans 8192 : %s\n",lens[t],used,fit>0?"oui":"NON");}
 e->destroy(p);return 0;}
