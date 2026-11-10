#define main previous_sample_test_main
#include "sample_test.c"
#undef main
static void drain(PT*p){char key[64];int n=0;while(mpc_engine_changed(p,key,sizeof key)>0)assert(++n<10000);}
int main(void){char path[]="/tmp/pitonic-lcd-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);wav(path,2,16,0,32000,3200);int status;Sample*s=sp_read(path,&status);assert(s&&status==2&&s->channels==2);
 for(int x=0;x<SP_LCD_COLS;x++){assert(s->lcd[0][x]==14);assert(s->lcd[1][x]==210);}
 PT*p=create(NULL);p->samples->current[0]=s;atomic_store(&p->samples->status[0],2);set(p,"source","1");drain(p);char text[64];assert(get(p,"sample_stereo",text,sizeof text)>0&&!strcmp(text,"1"));assert(get(p,"sample_lcd0",text,sizeof text)>0&&atoi(text)==14);assert(get(p,"sample_lcd288",text,sizeof text)>0&&atoi(text)==210);
 for(int rep=0;rep<120;rep++){set(p,"tone_q1",rep&1?"0.25":"0.75");set(p,"tone_q0",rep&1?"0.4":"0.6");char key[64];while(mpc_engine_changed(p,key,sizeof key)>0)assert(strncmp(key,"sample_lcd",10)&&strcmp(key,"sample_stereo"));}
 assert(s->lcd[0][0]==14&&s->lcd[1][0]==210);set(p,"select0","1");char key[64];while(mpc_engine_changed(p,key,sizeof key)>0)assert(strncmp(key,"sample_lcd",10));
 /* A two-second file with a short impulse must fill the LCD overview.
  * The stored PCM and frame count stay intact; START is display-mapped. */
 destroy(p);FILE*f=fopen(path,"wb");assert(f);unsigned count=64000;fwrite("RIFF",1,4,f);u32(f,36+count*2);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,1);u32(f,32000);u32(f,64000);u16(f,2);u16(f,16);fwrite("data",1,4,f);u32(f,count*2);for(unsigned k=0;k<count;k++)u16(f,k<3200?16384:0);fclose(f);
 s=sp_read(path,&status);assert(s&&s->frames==count&&s->lcd_frames==3360&&s->data[3199]==.5f&&s->data[3200]==0);p=create(NULL);p->samples->current[0]=s;atomic_store(&p->samples->status[0],2);set(p,"source","1");set(p,"sample_start","2.5");drain(p);assert(get(p,"sample_marker",text,sizeof text)>0&&fabsf(strtof(text,0)-47.61905f)<.001f);set(p,"sample_start","50");assert(get(p,"sample_marker",text,sizeof text)>0&&fabsf(strtof(text,0)-99.9f)<.001f);assert(p->start[0][0]==50);assert(s->lcd[0][270]!=0&&s->lcd[0][287]==0);destroy(p);unlink(path);puts("PASS LCD worker cache: real per-channel signed PCM extrema, normalized display only, stable waveform during 120 TUNE/START changes, same-voice reselection does not resend preview");}
