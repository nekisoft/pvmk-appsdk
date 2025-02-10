/* Copyright 1996 by Robert Morgan of Channel 7
   Sound Interface */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "d_global.h"
#include "d_misc.h"
#include "d_disk.h"
#include "audio.h"
#include "import.h"
#include "d_video.h"
#include "protos.h"
#include "r_refdef.h"
#include "d_ints.h"


/**** CONSTANTS ****/

#define MAXEFFECTS 8
#define NUMTRACKS effecttracks
#define MAXCACHESIZE (MAXEFFECTS * 2 + 4)
#define MAXSOUNDS 90
#define MAXSOUNDDIST 384
#define MSD (MAXSOUNDDIST << FRACBITS)
#define DEFAULTVRDIST 157286
#define DEFAULTVRANGLE 4


/**** TYPES ****/

typedef struct cache_s
{
    Sample* CSample;  // sound effect data
    int using;        // number of tracks using this sample
    int sfx;          // sound effect number
    int usage;        // how many times this sample used
} cache_t;


/**** VARIABLES ****/

boolean   MusicPresent, MusicPlaying, MusicSwapChannels;
SoundCard SC;
DSM*      M;
int       MusicError, EffectChan, CurrentChan, FXLump;
// int       StaticChan;
int CacheSize;
int SampleX[MAXEFFECTS], SampleY[MAXEFFECTS], SampleVol[MAXEFFECTS], SampleBal[MAXEFFECTS];
// int       StaticX[MAXSTATIC], StaticY[MAXSTATIC], StaticS[MAXSTATIC];
int     DVolume[MAXSOUNDDIST], MusicVol;
cache_t Cache[MAXCACHESIZE];
int     CacheIndex[MAXCACHESIZE];
int     effecttracks;
// int       statictracks=0;


/**** FUNCTIONS ****/

void StopMusic(void)
{
    int i;

    if (MusicError)
        return;
    if (MusicPlaying)
    {
        if (!netmode)
            for (i = MusicVol; i > 0; i -= 3) /* fade out */
            {
                if (i < 0)
                    i = 0;
                dSetMusicVolume(i);
                Wait(1);
            }
        dStopMusic();
        MusicPlaying = false;
        dFreeModule(M);
    }
    if (netmode)
        NetGetData();
}


void InitSound(void)
{
    int     i;
    fixed_t s, sfrac;
    boolean autodetect, noconfig;

    MusicPresent = false;
    dRegisterDrivers();
    autodetect = false;
    noconfig   = false;
    if (dLoadSetup(&SC, "SETUP.CFG")) /* load config file */
    {
        noconfig = true;
        printf("Sound:\tSETUP.CFG not found\n");
        printf("\tAuto-Detection=");
        autodetect = dAutoDetect(&SC);

        SC.ambientlight = 2048;  // load all defaults
        SC.violence     = true;
        SC.animation    = true;
        SC.musicvol     = 100;
        SC.sfxvol       = 128;
        SC.ckeys[0]     = scanbuttons[bt_run];
        SC.ckeys[1]     = scanbuttons[bt_jump];
        SC.ckeys[2]     = scanbuttons[bt_straf];
        SC.ckeys[3]     = scanbuttons[bt_fire];
        SC.ckeys[4]     = scanbuttons[bt_use];
        SC.ckeys[5]     = scanbuttons[bt_useitem];
        SC.ckeys[6]     = scanbuttons[bt_asscam];
        SC.ckeys[7]     = scanbuttons[bt_lookup];
        SC.ckeys[8]     = scanbuttons[bt_lookdown];
        SC.ckeys[9]     = scanbuttons[bt_centerview];
        SC.ckeys[10]    = scanbuttons[bt_slideleft];
        SC.ckeys[11]    = scanbuttons[bt_slideright];
        SC.ckeys[12]    = scanbuttons[bt_invleft];
        SC.ckeys[13]    = scanbuttons[bt_invright];
        SC.inversepan   = false;
        SC.screensize   = 0;
        SC.camdelay     = 35;
        SC.effecttracks = 4;
        SC.mouse        = 1;
        SC.joystick     = 0;

        SC.chartype    = 0;
        SC.socket      = 1234;
        SC.numplayers  = 2;
        SC.serplayers  = 1;
        SC.com         = 1;
        SC.rightbutton = bt_north;
        SC.leftbutton  = bt_fire;
        SC.joybut1     = bt_fire;
        SC.joybut2     = bt_straf;
        strncpy(SC.dialnum, "           ", 12);
        strncpy(SC.netname, "           ", 12);
        SC.netmap           = 22;
        SC.netdifficulty    = 2;
        SC.mousesensitivity = 32;
        SC.turnspeed        = 8;
        SC.turnaccel        = 2;

        SC.vrhelmet = 0;
        SC.vrangle  = DEFAULTVRANGLE;
        SC.vrdist   = DEFAULTVRDIST;

        lighting    = 1;
        changelight = SC.ambientlight;

        if (autodetect)
        {
            printf("Failed\n");
            MusicError = 1;
            return;
        }
        else
            printf("Success\n");
    }

    MusicSwapChannels = SC.inversepan;

    scanbuttons[bt_run]        = SC.ckeys[0];
    scanbuttons[bt_jump]       = SC.ckeys[1];
    scanbuttons[bt_straf]      = SC.ckeys[2];
    scanbuttons[bt_fire]       = SC.ckeys[3];
    scanbuttons[bt_use]        = SC.ckeys[4];
    scanbuttons[bt_useitem]    = SC.ckeys[5];
    scanbuttons[bt_asscam]     = SC.ckeys[6];
    scanbuttons[bt_lookup]     = SC.ckeys[7];
    scanbuttons[bt_lookdown]   = SC.ckeys[8];
    scanbuttons[bt_centerview] = SC.ckeys[9];
    scanbuttons[bt_slideleft]  = SC.ckeys[10];
    scanbuttons[bt_slideright] = SC.ckeys[11];
    scanbuttons[bt_invleft]    = SC.ckeys[12];
    scanbuttons[bt_invright]   = SC.ckeys[13];

    lighting        = 1;
    changelight     = SC.ambientlight;
    playerturnspeed = SC.turnspeed;
    turnunit        = SC.turnaccel;

    effecttracks = SC.effecttracks;

    if (SC.ID == ID_NONE)
    {
        MusicError = 2;
        return;
    } /* report info */
    else
    {
        if (noconfig == false)
            printf("Sound:");
        printf("\tIRQ=%i DMA=%i Rate=%i\n", SC.IrqLine, SC.DmaChannel, SC.SampleRate);
    }
    printf("\tDriver=%s\n", SC.DriverName);
    printf("\tInit=");
    if (dInit(&SC)) /* initialize the card */
    {
        printf("Sound Card Init failure\n");
        MusicError = 3;
        return;
    }
    atexit((void (*)(void)) dDone);
    printf("Success\n");
    MusicPresent = true;
    FXLump       = CA_GetNamedNum("SOUNDEFFECTS") + 1;
    if (FXLump == -1)
        MS_Error("InitSound: SOUNDEFFECTS lump not found in BLO file.");
    if (SC.ID != ID_GUS)
        CacheSize = NUMTRACKS * 2 + 4;
    else
        CacheSize = NUMTRACKS;
    memset(Cache, 0, sizeof(Cache));
    for (i = 0; i < NUMTRACKS; i++)
        CacheIndex[i] = -1;
    for (i = 0; i < CacheSize; i++)
        Cache[i].sfx = -1;
    s     = 63 << FRACBITS;
    sfrac = FIXEDDIV(s, MAXSOUNDDIST << FRACBITS);
    for (i = 0; i < MAXSOUNDDIST; i++, s -= sfrac) /* compute volume table */
        DVolume[i] = s >> FRACBITS;
    SetVolumes(SC.musicvol, SC.sfxvol);
}


Sample* GetCache(int n, int chan)
{
    int index, min, minindex;

    chan -= M->Header.NumTracks;
    index = 0;
    while (index < CacheSize && Cache[index].sfx != n)
        index++;  // look for duplicate
    if (index == CacheSize)
    {
        min      = 0x7FFFFFFF;
        minindex = 0x7FFFFFFF;
        for (index = 0; index < CacheSize; index++)
            if (Cache[index].using == 0 && Cache[index].usage < min)
            {
                min      = Cache[index].usage;
                minindex = index;
            }
        if (minindex == 0x7FFFFFFF)
            MS_Error("GetCache: No minimum Using");
        index = minindex;
    }
    if (Cache[index].sfx == n)
    {
        Cache[index].using ++;
        CacheIndex[chan] = index;
        if (Cache[index].usage < 32)
            Cache[index].usage++;  // have to limit (otherwise full of weapon sounds)
    }
    else
    {
        if (Cache[index].CSample)
            dFreeSample(Cache[index].CSample);
        Cache[index].sfx = n;
        lseek(cachehandle, infotable[FXLump + n].filepos, SEEK_SET);
        Cache[index].CSample = dLoadSampleFile(cachehandle, infotable[FXLump + n].size);
        if (Cache[index].CSample == NULL)
            MS_Error("GetCache: Error Loading WAV File: %i, %s", n, dErrorMsg[dError]);
        CacheIndex[chan] = index;
        Cache[index].using ++;
        Cache[index].usage = 1;
    }
    return Cache[index].CSample;
}


void FreeCache(int chan)
{
    chan -= M->Header.NumTracks;
    if (CacheIndex[chan] > -1)
        Cache[CacheIndex[chan]].using --;
}


void PlaySong(char* sname, int pattern)
{
    MHdr* P;
    int   format;

    if (MusicError)
        return; /* stop other music first */
    StopMusic();

    if (sname[strlen(sname) - 3] == 'm' || sname[strlen(sname) - 3] == 'M')
        format = FORM_MOD;
    else
        format = FORM_S3M;
    if (!(M = dImportModule(sname, format)))
        MS_Error("PlaySong: Error Loading S3M File: %s, %s", sname, dErrorMsg[dError]);
    dSetupVoices(M->Header.NumTracks + NUMTRACKS, M->Header.MasterVolume);
    dPlayMusic(M);
    if (netmode)
        NetGetData();
    if (pattern)
    {
        P            = dGetMusicStruc();
        P->OrderPos  = pattern;
        P->PattRow   = 0;
        P->BreakFlag = PB_JUMP;
    }
    SetVolumes(SC.musicvol, SC.sfxvol);
    if (netmode)
        NetGetData();
    EffectChan = M->Header.NumTracks;
    // StaticChan=EffectChan+effecttracks;
    MusicPlaying = true;
}


void SoundEffect(int n, int variation, fixed_t x, fixed_t y)
{
    int     x1, y1, z, d1, Chan, vol;
    fixed_t d, viewsin, viewcos;
    Sample* s;

    if (MusicError || SC.sfxvol == 0)
        return;
    x1 = (x - player.x) >> FRACTILESHIFT; /* don't play if too far */
    y1 = (y - player.y) >> FRACTILESHIFT;
    z  = x1 * x1 + y1 * y1;
    if (z >= MAXSOUNDDIST)
        return;
    SampleX[CurrentChan] = x >> FRACTILESHIFT;
    SampleY[CurrentChan] = y >> FRACTILESHIFT;
    viewcos              = costable[player.angle];
    viewsin              = sintable[player.angle]; /* compute left,right pan value */
    d = -FIXEDMUL(y1 << FRACTILESHIFT, viewcos) - FIXEDMUL(x1 << FRACTILESHIFT, viewsin);
    d = FIXEDDIV(d, MSD) * 0x40;
    if (MusicSwapChannels)
        d1 = (d >> FRACBITS) + 0x40;
    else
        d1 = -(d >> FRACBITS) + 0x40;
    if (d1 < 0)
        d1 = 0;
    else if (d1 > 0x80)
        d1 = 0x80;
    Chan = CurrentChan + EffectChan;
    dStopVoice(Chan);  // ok to stop old one now, we done
    FreeCache(Chan);
    s = GetCache(n, Chan);
    dPlayVoice(Chan, s); /* play it */
    vol = DVolume[z];
    dSetVoiceVolume(Chan, vol); /* set new volume */
    SampleVol[CurrentChan] = vol;
    dSetVoiceBalance(Chan, d1); /* set new pan */
    SampleBal[CurrentChan] = d1;
    if (midgetmode)
        dSetVoiceFreq(
            Chan,
            rint((float) s->Rate * 2
                 * ((float) ((MS_RndT() & variation) + 100 - (variation >> 1)) / (float) 100)));
    else
        dSetVoiceFreq(
            Chan,
            rint((float) s->Rate
                 * ((float) ((MS_RndT() & variation) + 100 - (variation >> 1)) / (float) 100)));
    ++CurrentChan;
    if (CurrentChan == effecttracks)
        CurrentChan = 0; /* go to next channel */
}


void StaticSoundEffect(int n, fixed_t x, fixed_t y)
{
	(void)n; (void)x; (void)y;
    /* int x1, y1, z1, d1, i, x2, y2, z2, Chan;
     fixed_t d, viewsin, viewcos;

     if (MusicError) return;
     if (SC.sfxvol==0) return;
     x1=(x-player.x)>>FRACTILESHIFT; // don't play if too far
     y1=(y-player.y)>>FRACTILESHIFT;
     z1=x1*x1 + y1*y1;
     if (z1>=MAXSOUNDDIST) return;
     Chan=StaticChan;
     for(i=0;i<statictracks;i++,Chan++)
      {
       x2=(StaticX[i]-player.x)>>FRACTILESHIFT;
       y2=(StaticY[i]-player.y)>>FRACTILESHIFT;
       z2=x2*x2+y2*y2;
       if (z1<z2)
        {
         StaticX[i]=x>>FRACTILESHIFT;
         StaticY[i]=y>>FRACTILESHIFT;
         StaticS[i]=n;
         viewcos=costable[player.angle];
         viewsin=sintable[player.angle]; // compute left,right pan value
         d=-FIXEDMUL(y1<<FRACTILESHIFT,viewcos)-FIXEDMUL(x1<<FRACTILESHIFT,viewsin);
         d=FIXEDDIV(d,MSD)*0x40;
         if (MusicSwapChannels) d1=(d>>FRACBITS) + 0x40;
          else d1=-(d>>FRACBITS) + 0x40;
         if (d1<0) d1=0;
          else if (d1>0x80) d1=0x80;
         dStopVoice(Chan);               // ok to stop old one now, we done
         FreeCache(Chan);
         dPlayVoice(Chan,GetCache(n,Chan)); // play it
         dSetVoiceVolume(Chan,DVolume[z1]); // set new volume
         dSetVoiceBalance(Chan,d1);         // set new pan
         break;
         }
       } */
}


void UpdateSound(void)
{
    int     x1, y1, z, i, px, py, d1, Chan, vol;
    fixed_t d, viewsin, viewcos;

    if (MusicError)
        return;

    if (MusicPlaying)
    {
        if (dGetMusicStatus() == PS_STOPPED)
            dPlayMusic(M);
    }
    px      = player.x >> FRACTILESHIFT; /* precomputations */
    py      = player.y >> FRACTILESHIFT;
    viewcos = costable[player.angle];
    viewsin = sintable[player.angle];
    Chan    = EffectChan;
    for (i = 0; i < effecttracks; i++, Chan++)
    {
        x1 = SampleX[i] - px;
        y1 = SampleY[i] - py;
        z  = x1 * x1 + y1 * y1;
        if (z >= MAXSOUNDDIST || z < 0) /* check for too far */
        {
            if (SampleVol[i] != 0)
            {
                dSetVoiceVolume(Chan, 0);
                SampleVol[i] = 0;
            }
            continue;
        }
        else
        {
            vol = DVolume[z];
            if (SampleVol[i] != vol)
            {
                dSetVoiceVolume(Chan, vol); /* reset volume */
                SampleVol[i] = vol;
            }
        }
        d = -FIXEDMUL(y1 << FRACTILESHIFT, viewcos) - FIXEDMUL(x1 << FRACTILESHIFT, viewsin);
        d = FIXEDDIV(d, MSD) * 0x40;
        if (MusicSwapChannels)
            d1 = (d >> FRACBITS) + 0x40;
        else
            d1 = -(d >> FRACBITS) + 0x40;
        if (d1 < 0)
            d1 = 0;
        else if (d1 > 0x80)
            d1 = 0x80;
        if (SampleBal[i] != d1)
        {
            dSetVoiceBalance(Chan, d1); /* reset pan value */
            SampleBal[i] = d1;
        }
    }
    /* Chan=StaticChan;
     for(i=0;i<statictracks;i++,Chan++)
      {
       if (dGetVoiceStatus(Chan)==PS_STOPPED)
        {
         StaticX[i]=100;
         StaticY[i]=100;
         dStopVoice(Chan);
         continue;
         }
       x1=StaticX[i]-px;
       y1=StaticY[i]-py;
       z=x1*x1 + y1*y1;
       if (z>=MAXSOUNDDIST || z<0)         // check for too far
        {
         dSetVoiceVolume(Chan,0);
         continue;
         }
       else dSetVoiceVolume(Chan,DVolume[z]);  // reset volume
       d=-FIXEDMUL(y1<<FRACTILESHIFT,viewcos)-FIXEDMUL(x1<<FRACTILESHIFT,viewsin);
       d=FIXEDDIV(d,MSD)*0x40;
       if (MusicSwapChannels) d1=(d>>FRACBITS) + 0x40;
        else d1=-(d>>FRACBITS) + 0x40;
       if (d1<0) d1=0;
        else if (d1>0x80) d1=0x80;
       dSetVoiceBalance(Chan,d1);  // reset pan value
       } */
}


void SetVolumes(int music, int fx)
{
    if (MusicError)
        return;
    if (music > 255)
        music = 255;
    dSetMusicVolume(music);
    MusicVol = music;
    if (fx > 255)
        fx = 255;
    dSetSoundVolume(fx);
    SC.musicvol = music;
    SC.sfxvol   = fx;
}
