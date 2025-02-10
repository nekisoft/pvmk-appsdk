/****************************************************************************
 *
 *                   Digital Sound Interface Kit (DSIK)
 *                            Version 2.00
 *
 *                           by Carlos Hasan
 *
 * Filename:     audio.h
 * Version:      Revision 1.1
 *
 * Language:     WATCOM C
 * Environment:  IBM PC (DOS/4GW)
 *
 * Description:  Audio Interface header file.
 *
 * Revision History:
 * ----------------
 *
 * Revision 1.1  94/11/05  12:13:39  chv
 * Added function to return the amount of free soundcard memory
 *
 * Revision 1.0  94/09/25  15:28:25  chv
 * Initial revision
 *
 ****************************************************************************/

#ifndef __AUDIO_H
#define __AUDIO_H

#include <stdint.h> //pvmk

/* Misc defines */

#define MAXVOICES 32
#define MAXTRACKS 16
#define MAXSAMPLES 256
#define MAXORDERS 128
#define MINPERIOD 28
#define MAXPERIOD 6848
#define MIDCPERIOD 428
#define MIDCFREQ 8363

/* Soundcards ID values */

#define ID_NONE 0
#define ID_SB 1
#define ID_SB201 2
#define ID_SBPRO 3
#define ID_SB16 4
#define ID_PAS 5
#define ID_PASPLUS 6
#define ID_PAS16 7
#define ID_WSS 8
#define ID_GUS 9
#define ID_PVMK 92 //pvmk - dummy soundcard as one systemcall handles it all
#define ID_DEBUG 255

/* Soundcards capabilities bit flags */

#define AF_8BITS 0x00
#define AF_16BITS 0x01
#define AF_MONO 0x00
#define AF_STEREO 0x02
#define AF_NODRAM 0x00
#define AF_DRAM 0x04

/* Stereo panning values */

#define PAN_LEFT 0x00
#define PAN_MIDDLE 0x40
#define PAN_RIGHT 0x80
#define PAN_SURROUND 0xA4

/* Internal lowlevel audio messages bitflags */

#define AM_SAMPLE 0x01
#define AM_KEYON 0x02
#define AM_KEYOFF 0x04
#define AM_VOLUME 0x08
#define AM_BALANCE 0x10
#define AM_SETCTL 0x20
#define AM_GETCTL 0x40
#define AM_PAUSE 0x80

/* Music playing status values */

#define PS_STOPPED 0
#define PS_PLAYING 1
#define PS_PAUSED 2

/* Music pattern break modes */

#define PB_NONE 0
#define PB_BREAK 1
#define PB_JUMP 2
#define PB_HOLD 3
#define PB_TRACE 4

/* RIFF/DSMF block identifier values */

#define ID_RIFF 0x46464952L
#define ID_DSMF 0x464D5344L
#define ID_SONG 0x474E4F53L
#define ID_INST 0x54534E49L
#define ID_PATT 0x54544150L

/* RIFF/WAVE block identifier values */

#define ID_WAVE 0x45564157L
#define ID_FMT 0x20746D66L
#define ID_DATA 0x61746164L

/* RIFF/DSMF INST digital samples bit flags */

#define SF_LOOPED 0x01
#define SF_UNSIGNED 0x00
#define SF_SIGNED 0x02
#define SF_8BITS 0x00
#define SF_16BITS 0x04
#define SF_DELTA 0x40
#define SF_LIBRARY 0x80

/* RIFF/WAVE sample format */

#define WAVE_FMT_PCM 1

/* Error values */

#define ERR_OK 0
#define ERR_FORMAT 1
#define ERR_NOFILE 2
#define ERR_FILEIO 3
#define ERR_NOMEM 4
#define ERR_NODRAM 5

#ifdef __cplusplus
extern "C"
{
#endif

    /* General typedefs */

    typedef uint32_t dword; //pvmk

    /* Internal lowlevel audio drivers structure */

//#pragma pack(1)

    typedef struct
    {
        dword Magic;
        dword Next;
        byte  ID;
        byte  Modes;
        char  Name[32];
        word  Port;
        byte  IrqLine;
        byte  DmaChannel;
        word  MinRate;
        word  MaxRate;
        word  BufferLength;
        void* ProcTablePtr;
        void* DriverPtr;
        word* ParmTablePtr;
    } __attribute__((packed)) Driver;

    /* Internal soundcards configuration structure */

    typedef struct SoundCard_s
    {
        byte ID;
        byte Modes;
        word Port;
        byte IrqLine;
        byte DmaChannel;
        word SampleRate;
        char DriverName[16];
        char inversepan;
        char ckeys[14];
        char effecttracks;
        int  musicvol;
        int  sfxvol;
        int  ambientlight;
        int  camdelay;
        int  screensize;
        char animation;
        char violence;
        char joystick;
        char mouse;
        int  chartype;
        int  socket;
        int  numplayers;
        char dialnum[13];
        int  com;
        int  serplayers;
        char netname[13];
        word jcenx, jceny, xsense, ysense;
        int  rightbutton, leftbutton;
        int  joybut1, joybut2;
        int  netmap, netdifficulty;
        int  mousesensitivity;
        int  turnspeed, turnaccel;
        int  vrhelmet;
        int  vrangle;
        int  vrdist;
    } __attribute__((packed)) SoundCard;

    /* RIFF file and block headers structures */

    typedef struct
    {
        dword ID;
        dword Length;
        dword Type;
    } __attribute__((packed)) RiffHeader;

    typedef struct
    {
        dword ID;
        dword Length;
    } __attribute__((packed)) RiffBlock;

    /* RIFF/WAVE fmt block structure */

    typedef struct
    {
        word  Format;
        word  Channels;
        dword SampleRate;
        dword BytesPerSecond;
        word  BlockAlign;
        word  BitsPerSample;
    } __attribute__((packed)) WaveFmt;

    /* RIFF/DSMF SONG block structure */

    typedef struct
    {
        char ModuleName[28];
        word FileVersion;
        word Flags;
        word OrderPos;
        word ReStart;
        word NumOrders;
        word NumSamples;
        word NumPatterns;
        word NumTracks;
        byte GlobalVolume;
        byte MasterVolume;
        byte InitTempo;
        byte InitBPM;
        byte ChanMap[MAXTRACKS];
        byte Orders[MAXORDERS];
    } __attribute__((packed)) Song;

    /* RIFF/DSMF INST block structure */

    typedef struct
    {
        char  FileName[13];
        word  Flags;
        byte  Volume;
        dword Length;
        dword LoopStart;
        dword LoopEnd;
        void* DataPtr;
        word  Rate;
        word  Voice;
        char  SampleName[28];
    } __attribute__((packed)) Sample;

    /* RIFF/DSMF PATT block structure */

    typedef struct
    {
        word Length;
        byte Data[1];
    } __attribute__((packed)) Pattern;

    /* Internal RIFF/DSMF music module structure */

    typedef struct
    {
        Song      Header;
        Sample**  Samples;
        Pattern** Patterns;
    } __attribute__((packed)) DSM;

    /* Internal music structures */

    typedef struct
    {
        byte Note;    /* note index */
        byte Sample;  /* sample number */
        byte Volume;  /* volume level */
        byte Balance; /* balance */
        word Effect;  /* Protracker command */
        word Rate;    /* middle-C finetune frequency */
        byte VUMeter; /* volume unit meter */
        byte Flags;   /* audio message bitflags */
        byte Reserved[38];
    } __attribute__((packed)) MTrk;

    typedef struct
    {
        dword MusicVolume;       /* music volume */
        dword SoundVolume;       /* sound effects volume */
        MTrk  Tracks[MAXVOICES]; /* track structures */
        byte  NumTracks;         /* number of active tracks */
        byte  NumVoices;         /* number of active voices */
        byte  OrderPos;          /* order position */
        byte  OrderLen;          /* order length */
        byte  ReStart;           /* restart position */
        byte  PattNum;           /* pattern number */
        byte  PattRow;           /* pattern row */
        byte  BreakFlag;         /* break pattern mode */
        byte  Tempo;             /* tempo */
        byte  TempoCount;        /* tempo counter */
        byte  BPM;               /* beats per minute */
        byte  SyncMark;          /* synchronization mark */
        byte  Status;            /* music status */
        byte  DriverStatus;      /* audio driver status */
        void* PattPtr;           /* internal pattern pointer */
        DSM*  SongPtr;           /* module pointer */
    } __attribute__((packed)) MHdr;

//#pragma pack(4)

    /* External lowlevel audio drivers */
	#define dRegisterDrivers() { }

    /* Global error variable */

    extern int   dError;
    extern char* dErrorMsg[];

    /* Audio interface API prototypes */

    void    dRegisterDriver(Driver* DriverPtr);
    Driver* dGetDriverStruc(int DriverId);
    int     dGetDriverFlags(void);
    int     dInit(SoundCard* SC);
    int     dDone(void);
    void    dPoll(void);
    void    dSetupVoices(int NumVoices, int MasterVolume);
    int32_t    dMemAlloc(Sample* SampPtr);
    void    dMemFree(Sample* SampPtr);
    int32_t    dMemAvail(void);
    void    dSetMusicVolume(int Volume);
    void    dSetSoundVolume(int Volume);
    int     dPlayMusic(DSM* MusicPtr);
    int     dPlayPatterns(DSM* MusicPtr, int OrderPos, int OrderEnd);
    void    dStopMusic(void);
    int     dPauseMusic(void);
    int     dResumeMusic(void);
    int     dGetMusicStatus(void);
    MHdr*   dGetMusicStruc(void);
    void    dPlayVoice(int Voice, Sample* SampPtr);
    void    dStopVoice(int Voice);
    void    dSetVoiceFreq(int Voice, int Freq);
    void    dSetVoiceVolume(int Voice, int Volume);
    void    dSetVoiceBalance(int Voice, int Balance);
    int32_t    dGetVoicePos(int Voice);
    int     dGetVoiceStatus(int Voice);

    int     dAutoDetect(SoundCard* SC);
    DSM*    dLoadModule(char* Filename);
    void    dFreeModule(DSM* MusicPtr);
    Sample* dLoadSample(char* Filename);
    void    dFreeSample(Sample* SampPtr);
    DSM*    dLoadModuleFile(int Handle, int32_t Length);
    Sample* dLoadSampleFile(int Handle, int32_t Length);
    int     dLoadSetup(SoundCard* SC, char* Filename);
    int     dSaveSetup(SoundCard* SC, char* Filename);

#ifdef __cplusplus
}
#endif

/* Register calling conventions used by the API routines */
/*
#pragma aux dRegisterDriver "_*" parm[eax];
#pragma aux dGetDriverStruc "_*" parm[eax];
#pragma aux dGetDriverFlags "_*" parm[];
#pragma aux dInit "_*" parm[eax];
#pragma aux dDone "_*" parm[];
#pragma aux dPoll "_*" parm[];
#pragma aux dSetupVoices "_*" parm[eax][edx];
#pragma aux dMemAlloc "_*" parm[eax][edx];
#pragma aux dMemFree "_*" parm[eax];
#pragma aux dMemAvail "_*" parm[];
#pragma aux dSetMusicVolume "_*" parm[eax];
#pragma aux dSetSoundVolume "_*" parm[eax];
#pragma aux dPlayMusic "_*" parm[eax];
#pragma aux dPlayPatterns "_*" parm[eax][edx][ebx];
#pragma aux dStopMusic "_*" parm[];
#pragma aux dPauseMusic "_*" parm[];
#pragma aux dResumeMusic "_*" parm[];
#pragma aux dGetMusicStatus "_*" parm[];
#pragma aux dGetMusicStruc "_*" parm[];
#pragma aux dPlayVoice "_*" parm[eax][edx];
#pragma aux dStopVoice "_*" parm[eax];
#pragma aux dSetVoiceFreq "_*" parm[eax][edx];
#pragma aux dSetVoiceVolume "_*" parm[eax][edx];
#pragma aux dSetVoiceBalance "_*" parm[eax][edx];
#pragma aux dGetVoicePos "_*" parm[eax];
#pragma aux dGetVoiceStatus "_*" parm[eax];

#pragma aux dAutoDetect "_*" parm[eax];
#pragma aux dLoadModule "_*" parm[eax];
#pragma aux dFreeModule "_*" parm[eax];
#pragma aux dLoadSample "_*" parm[eax];
#pragma aux dFreeSample "_*" parm[eax];
#pragma aux dLoadModuleFile "_*" parm[eax][edx];
#pragma aux dLoadSampleFile "_*" parm[eax][edx];
#pragma aux dLoadSetup "_*" parm[eax][edx];
#pragma aux dSaveSetup "_*" parm[eax][edx];

#pragma library(audio);
*/

#endif
