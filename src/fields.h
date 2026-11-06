/* Generated from scripts/config.py. */
#pragma once
#define PT_FIELDS 24
typedef struct {const char *key,*name,*unit;float lo,hi,def;int options;} PTField;
static const PTField pt_fields[PT_FIELDS]={
{"freq","FREQUENCY","Hz",20,20000,55,0},
{"wave","WAVE MORPH","%",0,100,0,0},
{"attack","AMP ATTACK","ms",0,10000,0,0},
{"decay","AMP DECAY","ms",10,10000,260,0},
{"modmode","PITCH MODE","",0,2,0,3},
{"modamt","PITCH AMOUNT","st",-96,96,12,0},
{"modrate","PITCH TIME / RATE","mode",0,20000,60,0},
{"modvel","PITCH VELOCITY","%",0,200,0,0},
{"mix","NOISE MIX","%",0,100,12,0},
{"filter","FILTER MODE","",0,2,0,3},
{"cutoff","CUTOFF","Hz",20,18000,2500,0},
{"res","RESONANCE","",0.1,20,0.707,0},
{"natk","NOISE ATTACK","ms",0,10000,0,0},
{"ndcy","NOISE DECAY","ms",10,10000,120,0},
{"env","NOISE ENVELOPE","",0,2,0,3},
{"stereo","STEREO NOISE","",0,1,0,2},
{"drive","DRIVE","dB",0,50,5,0},
{"eqfreq","EQ FREQUENCY","Hz",20,18000,200,0},
{"eqgain","EQ GAIN","dB",-24,24,0,0},
{"level","LEVEL","%",0,100,80,0},
{"ovel","OSC VELOCITY","%",0,200,100,0},
{"nvel","NOISE VELOCITY","%",0,200,100,0},
{"pan","PAN","",-100,100,0,0},
{"choke","CHOKE GROUP","",0,3,0,4},
};
