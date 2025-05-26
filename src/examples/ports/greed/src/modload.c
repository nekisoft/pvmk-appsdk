/****************************************************************************
 *
 *                   Digital Sound Interface Kit (DSIK)
 *                            Version 2.00
 *
 *                           by Carlos Hasan
 *
 * Filename:     modload.c
 * Version:      Revision 1.0
 *
 * Language:     WATCOM C
 * Environment:  IBM PC (DOS/4GW)
 *
 * Description:  File loading routines for module and sample files.
 *
 * Revision History:
 * ----------------
 *
 * Revision 1.0  94/09/24  16:57:58  chv
 * Initial revision
 *
 ****************************************************************************/

#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include "d_global.h"
#include "audio.h"

int   dError;
char* dErrorMsg[] = { "Ok",
                      "Invalid or unsupported file format",
                      "Unable to open file",
                      "File is corrupted",
                      "Not enough system memory",
                      "Not enough soundcard memory" };


/****************************************************************************
 *
 * Function:     LoadSample
 * Parameters:   Handle  - file handle
 *               Length  - sample block length
 *
 * Returns:      Sample structure containing the WAVE digital sample
 *               or NULL when an error has occurred.
 *
 * Description:  Used to load RIFF/DSMF sample blocks.
 *
 ****************************************************************************/

static Sample* LoadSample(int Handle, int32_t Length)
{
	(void)Length;
    Sample* SampPtr;
    void*   DataPtr;

    if (!(SampPtr = (Sample*) malloc(sizeof(Sample))))
    {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (read(Handle, SampPtr, sizeof(Sample)) != sizeof(Sample))
    {
        dError = ERR_FILEIO;
        free(SampPtr);
        return NULL;
    }
    if (!SampPtr->Length || (SampPtr->Flags & SF_LIBRARY))
    {
        SampPtr->DataPtr = NULL;
        return SampPtr;
    }
    if (!(DataPtr = SampPtr->DataPtr = malloc(SampPtr->Length)))
    {
        dError = ERR_NOMEM;
        free(SampPtr);
        return NULL;
    }
    if (read(Handle, SampPtr->DataPtr, SampPtr->Length) != (int)(SampPtr->Length))
    {
        dError = ERR_FILEIO;
        free(DataPtr);
        free(SampPtr);
        return NULL;
    }
    if (!dMemAlloc(SampPtr))
    {
        dError = ERR_NODRAM;
        free(DataPtr);
        free(SampPtr);
        return NULL;
    }
    if (dGetDriverFlags() & AF_DRAM)
    {
        free(DataPtr);
    }
    if (!(SampPtr->Flags & SF_LOOPED))
    {
        SampPtr->LoopStart = SampPtr->LoopEnd = SampPtr->Length;
    }
    return SampPtr;
}


/****************************************************************************
 *
 * Function:     FreeSample
 * Parameters:   SampPtr - Sample structure pointer
 *
 * Description:  Releases memory used by digital samples.
 *
 ****************************************************************************/

static void FreeSample(Sample* SampPtr)
{
    if (SampPtr)
    {
        if (SampPtr->DataPtr && !(SampPtr->Flags & SF_LIBRARY))
        {
            dMemFree(SampPtr);
            if (!(dGetDriverFlags() & AF_DRAM))
                free(SampPtr->DataPtr);
        }
        free(SampPtr);
    }
}


/****************************************************************************
 *
 * Function:     LoadPattern
 * Parameters:   Handle  - file handle
 *               Length  - RIFF pattern block length
 *
 * Returns:      Pattern structure containing the music pattern data
 *               or NULL when an error has occurred.
 *
 * Description:  Used to load RIFF/DSMF pattern blocks.
 *
 ****************************************************************************/

static Pattern* LoadPattern(int Handle, int32_t Length)
{
    Pattern* PattPtr;

    if (!(PattPtr = (Pattern*) malloc(Length)))
    {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (read(Handle, PattPtr, Length) != Length)
    {
        dError = ERR_FILEIO;
        free(PattPtr);
        return NULL;
    }
    return PattPtr;
}


/****************************************************************************
 *
 * Function:     FreePattern
 * Parameters:   PattPtr - pattern data pointer
 *
 * Description:  Releases memory used by the patterns.
 *
 ****************************************************************************/

static void FreePattern(Pattern* PattPtr)
{
    if (PattPtr)
        free(PattPtr);
}


/****************************************************************************
 *
 * Function:     dLoadModuleFile
 * Parameters:   Handle      - DOS file handle
 *               Length      - file length
 *
 * Returns:      Music module address or NULL when an error has occurred
 *               while loading the file.
 *
 * Description:  Load RIFF/DSMF music modules from disk.
 *
 ****************************************************************************/

DSM* dLoadModuleFile(int Handle, int32_t Length)
{
    RiffHeader Header;
    RiffBlock  Block;
    DSM*       Module;
    int        SampNum, PattNum;

    if (!(Module = (DSM*) calloc(1, sizeof(DSM))))
    {
        dError = ERR_NOMEM;
        return NULL;
    }
    SampNum = PattNum = 0;

    if (read(Handle, &Header, sizeof(Header)) != sizeof(Header))
    {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (Header.ID != ID_RIFF || Header.Type != ID_DSMF)
    {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }
    Header.Length -= sizeof(Header.Type);
    if ((int)(sizeof(Header) + Header.Length) > Length)
    {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }
    while (Header.Length)
    {
        if (read(Handle, &Block, sizeof(Block)) != sizeof(Block))
        {
            dError = ERR_FILEIO;
            dFreeModule(Module);
            return NULL;
        }
        Header.Length -= sizeof(Block) + Block.Length;
        if (Block.ID == ID_SONG)
        {
            if (read(Handle, &Module->Header, Block.Length) != (ssize_t)Block.Length)
            {
                dError = ERR_FILEIO;
                dFreeModule(Module);
                return NULL;
            }
            Module->Samples  = (Sample**) calloc(Module->Header.NumSamples, sizeof(Sample*));
            Module->Patterns = (Pattern**) calloc(Module->Header.NumPatterns, sizeof(Pattern*));
            if (!Module->Samples || !Module->Patterns)
            {
                dError = ERR_NOMEM;
                dFreeModule(Module);
                return NULL;
            }
        }
        else if (Block.ID == ID_INST)
        {
            if (SampNum >= Module->Header.NumSamples)
            {
                dError = ERR_FORMAT;
                dFreeModule(Module);
                return NULL;
            }
            if (!(Module->Samples[SampNum++] = LoadSample(Handle, Block.Length)))
            {
                dFreeModule(Module);
                return NULL;
            }
        }
        else if (Block.ID == ID_PATT)
        {
            if (PattNum >= Module->Header.NumPatterns)
            {
                dError = ERR_FORMAT;
                dFreeModule(Module);
                return NULL;
            }
            if (!(Module->Patterns[PattNum++] = LoadPattern(Handle, Block.Length)))
            {
                dFreeModule(Module);
                return NULL;
            }
        }
        else
        {
            if (lseek(Handle, Block.Length, SEEK_CUR) < 0)
            {
                dError = ERR_FILEIO;
                dFreeModule(Module);
                return NULL;
            }
        }
    }
    return Module;
}


/****************************************************************************
 *
 * Function:     dFreeModule
 * Parameters:   Module  - music module pointer
 *
 * Description:  Releases all the resources used by the music module.
 *
 ****************************************************************************/

void dFreeModule(DSM* Module)
{
    int Index;

    if (Module)
    {
        if (Module->Samples)
        {
            for (Index = 0; Index < Module->Header.NumSamples; Index++)
                if (Module->Samples[Index])
                    FreeSample(Module->Samples[Index]);
            free(Module->Samples);
        }
        if (Module->Patterns)
        {
            for (Index = 0; Index < Module->Header.NumPatterns; Index++)
                if (Module->Patterns[Index])
                    FreePattern(Module->Patterns[Index]);
            free(Module->Patterns);
        }
        free(Module);
    }
}


/****************************************************************************
 *
 * Function:     dLoadSampleFile
 * Parameters:   Handle  - file handle
 *               Length  - sample length
 *
 * Returns:      Sample structure containing the WAVE digital sample
 *               or NULL when an error has occurred.
 *
 * Description:  Used to load RIFF/WAVE sample files.
 *
 ****************************************************************************/

Sample* dLoadSampleFile(int Handle, int32_t Length)
{
    Sample* SampPtr;
    void*   DataPtr;
    struct
    {
        RiffHeader Hdr;
        RiffBlock  Fmt;
        WaveFmt    F;
        RiffBlock  Data;
    } Wave;

    if (read(Handle, &Wave, sizeof(Wave)) != sizeof(Wave))
    {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Wave.Hdr.ID != ID_RIFF || Wave.Hdr.Type != ID_WAVE || Wave.Fmt.ID != ID_FMT
        || Wave.Data.ID != ID_DATA || Wave.F.Format != WAVE_FMT_PCM || Wave.F.Channels != 1
        || Wave.F.BitsPerSample != 8)
    {
        dError = ERR_FORMAT;
        return NULL;
    }


    if (sizeof(RiffBlock) + Wave.Hdr.Length > (unsigned)Length)
    {
        Wave.Data.Length = Length - sizeof(Wave);
        Wave.Hdr.Length  = sizeof(Wave);
    }

    if (!(SampPtr = (Sample*) calloc(1, sizeof(Sample))))
    {
        dError = ERR_NOMEM;
        return NULL;
    }

    SampPtr->Flags  = SF_8BITS | SF_UNSIGNED;
    SampPtr->Volume = 64;
    SampPtr->Length = SampPtr->LoopStart = SampPtr->LoopEnd = Wave.Data.Length;
    SampPtr->DataPtr                                        = NULL;
    SampPtr->Rate                                           = Wave.F.SampleRate;
    if (!SampPtr->Length)
    {
        return SampPtr;
    }

    DataPtr = malloc(SampPtr->Length);
    if (!DataPtr)
    {
        dError = ERR_NOMEM;
        free(SampPtr);
        return NULL;
    }
    SampPtr->DataPtr = DataPtr;

    if (read(Handle, DataPtr, SampPtr->Length) != (ssize_t)SampPtr->Length)
    {
        dError = ERR_FILEIO;
        free(DataPtr);
        free(SampPtr);
        return NULL;
    }
    if (!dMemAlloc(SampPtr))
    {
        dError = ERR_NODRAM;
        free(DataPtr);
        free(SampPtr);
        return NULL;
    }
    if (dGetDriverFlags() & AF_DRAM)
    {
        free(DataPtr);
    }
    return SampPtr;
}


/****************************************************************************
 *
 * Function:     dFreeSample
 * Parameters:   SampPtr - Sample pointer
 *
 * Description:  Releases the memory used by a WAVE sample.
 *
 ****************************************************************************/

void dFreeSample(Sample* SampPtr) { FreeSample(SampPtr); }


/****************************************************************************
 *
 * Function:     dLoadModule
 * Parameters:   Filename    - music module path filename
 *
 * Returns:      Music module address or NULL when an error has occurred
 *               while loading the file.
 *
 * Description:  Load RIFF/DSMF music modules from disk.
 *
 ****************************************************************************/

int filelength(int handle); //pvmk - implemented in import.c

DSM* dLoadModule(char* Filename)
{
    int  Handle;
    int32_t Length;
    DSM* Module;

    if ((Handle = open(Filename, O_RDONLY /*| O_BINARY*/)) < 0)
    {
        dError = ERR_NOFILE;
        return NULL;
    }
    if ((Length = filelength(Handle)) < 0)
    {
        dError = ERR_FILEIO;
        close(Handle);
        return NULL;
    }
    Module = dLoadModuleFile(Handle, Length);
    close(Handle);
    return Module;
}


/****************************************************************************
 *
 * Function:     dLoadSample
 * Parameters:   Filename    - music module path filename
 *
 * Returns:      Music module address or NULL when an error has occurred
 *               while loading the file.
 *
 * Description:  Load RIFF/WAVE sample files from disk.
 *
 ****************************************************************************/

Sample* dLoadSample(char* Filename)
{
    int     Handle;
    int32_t    Length;
    Sample* SampPtr;

    if ((Handle = open(Filename, O_RDONLY /*| O_BINARY*/)) < 0)
    {
        dError = ERR_NOFILE;
        return NULL;
    }
    if ((Length = filelength(Handle)) < 0)
    {
        dError = ERR_FILEIO;
        close(Handle);
        return NULL;
    }
    SampPtr = dLoadSampleFile(Handle, Length);
    close(Handle);
    return SampPtr;
}


/****************************************************************************
 *
 * Function:     dLoadSetup
 * Parameters:   SC          - soundcard structure
 *               Filename    - full config path filename
 *
 * Returns:      Zero value on success.
 *
 * Description:  Load the soundcard configuration parameters.
 *
 ****************************************************************************/

int dLoadSetup(SoundCard* SC, char* Filename)
{
	(void)SC;
	(void)Filename;
	return -1;
}


/****************************************************************************
 *
 * Function:     dSaveSetup
 * Parameters:   SC          - soundcard structure
 *               Filename    - full config path filename
 *
 * Returns:      Zero value on success.
 *
 * Description:  Save the soundcard configuration parameters.
 *
 ****************************************************************************/

int dSaveSetup(SoundCard* SC, char* Filename)
{
	(void)SC;
	(void)Filename;
	return -1;
}
