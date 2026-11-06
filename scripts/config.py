from pathlib import Path
B=Path(__file__).resolve().parents[1]
VERSION='1.0.0'
FOLDER='Timo - VST - πToniC'
# Physical units. All canonical voice parameters stay at fixed append-only indices.
FIELDS=[
 ('freq','FREQUENCY',20,20000,55,'Hz'),('wave','WAVE MORPH',0,100,0,'%'),
 ('attack','AMP ATTACK',0,10000,0,'ms'),('decay','AMP DECAY',10,10000,260,'ms'),
 ('modmode','PITCH MODE',0,2,0,['DECAY','SINE','RANDOM']),
 ('modamt','PITCH AMOUNT',-96,96,12,'st'),('modrate','PITCH TIME / RATE',0,20000,60,'mode'),
 ('modvel','PITCH VELOCITY',0,200,0,'%'),
 ('mix','NOISE MIX',0,100,12,'%'),('filter','FILTER MODE',0,2,0,['LP','BP','HP']),
 ('cutoff','CUTOFF',20,18000,2500,'Hz'),('res','RESONANCE',0.1,20,0.707,''),
 ('natk','NOISE ATTACK',0,10000,0,'ms'),('ndcy','NOISE DECAY',10,10000,120,'ms'),
 ('env','NOISE ENVELOPE',0,2,0,['EXP','LINEAR','CLAP']),('stereo','STEREO NOISE',0,1,0,['OFF','ON']),
 ('drive','DRIVE',0,50,5,'dB'),('eqfreq','EQ FREQUENCY',20,18000,200,'Hz'),
 ('eqgain','EQ GAIN',-24,24,0,'dB'),('level','LEVEL',0,100,80,'%'),
 ('ovel','OSC VELOCITY',0,200,100,'%'),('nvel','NOISE VELOCITY',0,200,100,'%'),
 ('pan','PAN',-100,100,0,''),('choke','CHOKE GROUP',0,3,0,['OFF','GROUP 1','GROUP 2','GROUP 3'])]

FX_FIELDS=[
 ('rev_on','REVERB',0,1,1,['OFF','ON']),('rev_size','SIZE',0,100,65,'%'),
 ('rev_decay','DECAY',.2,8,2.4,'s'),('rev_damp','DAMPING',0,100,45,'%'),('rev_return','RETURN',0,100,55,'%'),
 ('del_on','DELAY',0,1,1,['OFF','ON']),('del_sync','CLOCK',0,1,1,['FREE','SYNC']),
 ('del_div','TIME',0,4,2,['1/16','1/8','1/8 D','1/4','1/2']),('del_ms','TIME',20,1500,375,'ms'),
 ('del_feedback','FEEDBACK',0,90,35,'%'),('del_tone','TONE',0,100,55,'%'),('del_return','RETURN',0,100,50,'%')]
