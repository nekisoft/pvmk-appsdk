/****************************************************************************
 *
 *                   Digital Sound Interface Kit (DSIK)
 *                            Version 2.00
 *
 *                           by Carlos Hasan
 *
 * Filename:     import.h
 * Version:      Revision 1.3
 *
 * Language:     WATCOM C
 * Environment:  IBM PC (DOS/4GW)
 *
 * Description:  External music modules import routines.
 *
 * Revision History:
 * ----------------
 *
 * Revision 1.3  94/11/17  16:58:20  chv
 * Added Composer 669 and Scream Tracker 2.0 import routines
 *
 * Revision 1.2  94/11/08  15:10:42  chv
 * Added Amiga IFF/8SVX sample file import routines
 *
 * Revision 1.1  94/10/26  13:12:43  chv
 * Added Multitracker MTM modules import routines
 *
 * Revision 1.0  94/08/22  16:09:24  chv
 * Initial revision
 *
 ****************************************************************************/

#ifndef __IMPORT_H
#define __IMPORT_H

#ifndef __AUDIO_H
#include <audio.h>
#endif

/* supported file formats */

#define FORM_DSM 0x00 /* RIFF/DSMF module file */
#define FORM_MOD 0x01 /* Protracker/Fastracker modules */
#define FORM_S3M 0x02 /* Scream Tracker 3.0 modules */
#define FORM_MTM 0x03 /* Multitracker 1.0 modules */
#define FORM_669 0x04 /* Composer 669 modules */
#define FORM_STM 0x05 /* Scream Tracker 2.0 modules */

#define FORM_WAV 0x80 /* Windows WAVE PCM files */
#define FORM_VOC 0x81 /* Creative Labs Voice Files */
#define FORM_IFF 0x82 /* Amiga IFF/8SVX sample files */
#define FORM_RAW 0x83 /* 8-bit mono RAW sample files */


/* Protracker/Fastracker (MOD) file format */

#define MOD_MK 0x2E4B2E4DL
#define MOD_FLT4 0x34544C46L
#define MOD_FLT8 0x38544C46L
#define MOD_6CHN 0x4E484336L
#define MOD_8CHN 0x4E484338L

//#pragma pack(1)

typedef struct
{
    char SampleName[22];
    word Length;
    byte Finetune;
    byte Volume;
    word LoopStart;
    word LoopLength;
} __attribute__((packed)) MODSample;

typedef struct
{
    char      SongName[20];
    MODSample Samples[31];
    byte      NumOrders;
    byte      ReStart;
    byte      Orders[128];
    dword     Magic;
} __attribute__((packed)) MODSong;


/* Scream Tracker 3.0 (S3M) file format */

#define S3M_SCRM 0x4D524353L
#define S3M_SCRS 0x53524353L

typedef struct
{
    byte  Type;
    char  FileName[13];
    word  DataPtr;
    dword Length;
    dword LoopStart;
    dword LoopEnd;
    byte  Volume;
    byte  LibDisk;
    byte  Pack;
    byte  Flags;
    dword Rate;
    char  Pad0[12];
    char  SampleName[28];
    dword Magic;
} __attribute__((packed)) S3MSample;

typedef struct
{
    char  SongName[28];
    byte  EofMark;
    byte  SongType;
    word  Pad0;
    word  NumOrders;
    word  NumSamples;
    word  NumPatterns;
    word  Flags;
    word  Tracker;
    word  FileFormat;
    dword Magic;
    byte  GlobalVolume;
    byte  Tempo;
    byte  BPM;
    byte  MasterVolume;
    byte  UltraClick;
    byte  DefPan;
    byte  Pad1[8];
    word  Special;
    byte  ChanMap[32];
} __attribute__((packed)) S3MSong;


/* Multitracker 1.0 (MTM) file format */

#define MTM_MAGIC 0x104D544DL
#define MTM_MASK 0xF0FFFFFFL

typedef struct
{
    dword Magic;
    char  SongName[20];
    word  NumTracks;
    byte  LastPattern;
    byte  LastOrder;
    word  CommentLength;
    byte  NumSamples;
    byte  Flags;
    byte  BeatsPerTrack;
    byte  NumChans;
    byte  PanPos[32];
} __attribute__((packed)) MTMSong;

typedef struct
{
    char  SampleName[22];
    dword Length;
    dword LoopStart;
    dword LoopEnd;
    byte  Finetune;
    byte  Volume;
    byte  Flags;
} __attribute__((packed)) MTMSample;

typedef struct
{
    byte Data[192];
} __attribute__((packed)) MTMTrack;


/* Composer 669 file format */

#define GG9_MAGIC 0x6669

typedef struct
{
    word Magic;
    char SongName[108];
    byte NumSamples;
    byte NumPatterns;
    byte ReStart;
    byte Orders[128];
    byte Tempos[128];
    byte Breaks[128];
} __attribute__((packed)) GG9Song;

typedef struct
{
    char  FileName[13];
    dword Length;
    dword LoopStart;
    dword LoopEnd;
} __attribute__((packed)) GG9Sample;


/* Scream Tracker 2.0 (STM) modules */

#define STM_TRACKER "!Scream!"

typedef struct
{
    char FileName[13];
    byte LibDisk;
    word DataPtr;
    word Length;
    word LoopStart;
    word LoopEnd;
    byte Volume;
    byte Pad0;
    word Rate;
    byte Pad1[6];
} __attribute__((packed)) STMSample;

typedef struct
{
    char      SongName[20];
    char      Tracker[8];
    byte      EofMark;
    byte      FileType;
    word      TrackerVer;
    byte      InitTempo;
    byte      NumPatterns;
    byte      GlobalVolume;
    byte      Pad[13];
    STMSample Samples[31];
    byte      Orders[128];
} __attribute__((packed)) STMSong;


/* Creative Labs Voice (VOC) file format */

#define VOC_HDR "Creative Voice File\32"

#define VOC_TERM 0
#define VOC_DATA 1
#define VOC_CONTINUE 2
#define VOC_SILENCE 3
#define VOC_MARK 4
#define VOC_TEXT 5
#define VOC_REPEAT 6
#define VOC_ENDREPEAT 7
#define VOC_EXTINFO 8

#define VOC_FMT_8BITS 0
#define VOC_FMT_4BITS 1
#define VOC_FMT_26BITS 2
#define VOC_FMT_2BITS 3

#define VOC_MODE_MONO 0
#define VOC_MODE_STEREO 1

typedef struct
{
    byte Magic[20];
    word BlockPos;
    word Version;
    word VersionCheck;
} __attribute__((packed)) VocHeader;

typedef struct
{
    byte Type;
    byte Size[3];
} __attribute__((packed)) VocBlock;

typedef struct
{
    byte TimeConst;
    byte Format;
} __attribute__((packed)) VocData;

typedef struct
{
    word TimeConst;
    byte Pack;
    byte Mode;
} __attribute__((packed)) VocExtInfo;


/* Amiga IFF/8SVX file format */

#define IFF_FORM 0x4D524F46L
#define IFF_8SVX 0x58565338L
#define IFF_VHDR 0x52444856L
#define IFF_ANNO 0x4F4E4E41L
#define IFF_NAME 0x454D414EL
#define IFF_CHAN 0x4E414843L
#define IFF_BODY 0x59444F42L

#define WSWAP(w) ((((w) &0xFF) << 8) | (((w) >> 8) & 0xFF))
#define LSWAP(l) ((WSWAP((l) &0xFFFF) << 16) | (WSWAP((l) >> 16)))

typedef struct
{
    dword ID;
    dword Length;
    dword Type;
} __attribute__((packed)) IffHeader;

typedef struct
{
    dword ID;
    dword Length;
} __attribute__((packed)) IffBlock;

typedef struct
{
    byte Pad0[12];
    word Rate;
    byte Pad1;
    byte Format;
    byte Pad2[4];
} __attribute__((packed)) IffVHdr;

//#pragma noalign(IffChan)
typedef struct
{
    dword Channels;
} __attribute__((packed)) IffChan;

//#pragma pack(4)

#ifdef __cplusplus
extern "C"
{
#endif

    /* Import routines API prototypes */

    DSM*    dImportModule(char* Filename, int Form);
    Sample* dImportSample(char* Filename, int Form);
    DSM*    dImportModuleFile(int Handle, long Length, int Form);
    Sample* dImportSampleFile(int Handle, long Length, int Form);

#ifdef __cplusplus
}
#endif

/* Register calling convention used by the API routines */
/*
#pragma aux dImportModule "_*" parm[eax][edx];
#pragma aux dImportSample "_*" parm[eax][edx];
#pragma aux dImportModuleFile "_*" parm[eax][edx][ebx];
#pragma aux dImportSampleFile "_*" parm[eax][edx][ebx];
*/

#endif
