//pvmkaudio.c
//Audio replacements for PVMK port of GREED
//Bryan E. Topp <betopp@betopp.com> 2025

#include "d_global.h"
#include "audio.h"

/*
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
*/

int32_t dMemAlloc(Sample* SampPtr)
{
	(void)SampPtr;
	return 1;
}

void dMemFree(Sample* SampPtr)
{
	(void)SampPtr;
}

void dPlayVoice(int Voice, Sample* SampPtr)
{
	(void)Voice;
	(void)SampPtr;
}

int dGetDriverFlags(void)
{
	return 0;
}

void dPoll(void)
{
	
}

void dSetMusicVolume(int Volume)
{
	(void)Volume;
}

void dSetSoundVolume(int Volume)
{
	(void)Volume;
}

int dInit(SoundCard* SC)
{
	(void)SC;
	return 0;
}

int dDone(void)
{
	return 0;
}

void dStopMusic(void)
{
	
}

int dPlayMusic(DSM* MusicPtr)
{
	(void)MusicPtr;
	return 0;
}

MHdr pvmk_musicstruc;
MHdr *dGetMusicStruc(void)
{
	return &pvmk_musicstruc;
}

void dSetVoiceVolume(int Voice, int Volume)
{
	(void)Voice;
	(void)Volume;
}

void dSetVoiceBalance(int Voice, int Balance)
{
	(void)Voice;
	(void)Balance;
}

void dSetupVoices(int NumVoices, int MasterVolume)
{
	(void)NumVoices;
	(void)MasterVolume;
}

void dStopVoice(int Voice)
{
	(void)Voice;
}

void dSetVoiceFreq(int Voice, int Freq)
{
	(void)Voice;
	(void)Freq;
}

int dGetMusicStatus(void)
{
	return PS_STOPPED;
}
