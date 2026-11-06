#include <assert.h>
#include "../src/engine.c"
static void advance(PT*p,int frames){int16_t out[256];while(frames){int n=frames>128?128:frames;render(p,out,n);frames-=n;}}
int main(void){PT*p=create(0),*q=create(0);char key[80],state[32768];
assert(!p->activity[0]);trigger(p,0,100);assert(p->activity[0]&&!q->activity[0]);assert(mpc_engine_changed(p,key,sizeof key)&&!strcmp(key,"activity0"));assert(!mpc_engine_changed(p,key,sizeof key));
advance(p,3000);trigger(p,0,100);assert(!mpc_engine_changed(p,key,sizeof key));advance(p,4000);assert(p->activity[0]);advance(p,2174);assert(!p->activity[0]);assert(mpc_engine_changed(p,key,sizeof key)&&!strcmp(key,"activity0"));
change(p,0,PT_FIELDS,1);trigger(p,0,100);assert(!p->activity[0]);change(p,1,PT_FIELDS+1,1);trigger(p,2,100);assert(!p->activity[2]);trigger(p,1,100);assert(p->activity[1]);assert(p->selected==0);
get(p,"state",state,sizeof state);set(q,"state",state);assert(!q->activity[1]);set(p,"state",state);assert(!p->activity[1]);destroy(p);destroy(q);puts("PASS: activity onset, expiry, retrigger coalescing, mute/solo exclusion, independent instances and transient state");}
