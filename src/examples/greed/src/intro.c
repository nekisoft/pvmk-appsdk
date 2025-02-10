/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1996 by Softdisk Publishing                               */
/*                                                                         */
/* Original Design:                                                        */
/*  John Carmack of id Software                                            */
/*                                                                         */
/* Enhancements by:                                                        */
/*  Robert Morgan of Channel 7............................Main Engine Code */
/*  Todd Lewis of Softdisk Publishing......Tools,Utilities,Special Effects */
/*  John Bianca of Softdisk Publishing..............Low-level Optimization */
/*  Carlos Hasan..........................................Music/Sound Code */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "d_global.h"
#include "d_disk.h"
#include "d_misc.h"
#include "d_video.h"
#include "d_ints.h"
#include "r_refdef.h"
#include "r_public.h"
#include "d_font.h"
#include "protos.h"
#include "audio.h"


/**** CONSTANTS ****/

#ifdef GAME1
#define VERSION "1.000.P1"
#elif defined(GAME2)
#define VERSION "1.000.P2"
#elif defined(GAME3)
#define VERSION "1.000.P3"
#else
#define VERSION "1.000.040" "-PVMK"
#endif

#define DEMOVERSION "7.0"

char* charinfo[5][27] =  // character profiles
    {
{
        "CYBORG",
        ".",
        "NAME: TOBIAS LOCKE",
        ".",
        "RACE: HOMO SAPIEN",
        ".",
        "AGE: 27",
        ".",
        "HEIGHT: 6'1''",
        ".",
        "WEIGHT: 450 LBS",
        "(INCLUDING POWERED LIMBS)",
        ".",
        "BIO: CYBERNETICALLY ENHANCED HUMAN",
        ".",
        "BACKGROUND: BORN INTO THE LOTHLOS",
        "CASTE OF HUNTERS, TOBIAS WAS",
        "REBUILT AT THE AGE OF 22 FOLLOWING",
        "HIS INITIATION RITE INTO THE ELITE",
        "LOTH MAL ESCH, OR \"SCAVENGER",
        "BROOD.\"",
        "     HE WAS SOON RECRUITED BY THE",
        "GREEN QUARTER HUNT SQUAD AND",
        "ASSIGNED TO THE SCAVENGER VESSEL,",
        "RED HUNTER, FOR COMPETITION IN THE",
        "GAME.",
        ".",
},
{

        "LIZARD MAN",
        ".",
        "NAME: XITH",
        ".",
        "RACE: SAPIOSAURUS ROBUSTUS",
        ".",
        "AGE: 43",
        ".",
        "HEIGHT: 4'11''",
        ".",
        "WEIGHT: 100 LBS",
        ".",
        "BIO: ZOLLEESIAN LIZARD MAN",
        ".",
        "BACKGROUND: XITH WAS BORN BETWEEN",
        "LIZARD CLANS AND FROM BIRTH HAS",
        "BEEN AN OUTCAST AMONG HIS TRIBES.",
        "HE ENTERED THE GAME HOPING TO USE",
        "THE LIGHTNING QUICK SPEED INHERENT",
        "IN HIS RACE TO VINDICATE HIMSELF.",
        "     XITH WAS TRADED TO GREEN",
        "QUARTER AND SUBSEQUENTLY PLACED",
        "ABOARD THE RED HUNTER.",
        ".",
        ".",
        ".",
        ".",
},
{
        "MOOMAN",
        ".",
        "NAME: ALDUS KADEN",
        ".",
        "RACE: BRAHMAN ERECTUS",
        ".",
        "AGE: 35",
        ".",
        "HEIGHT: 7'0''",
        ".",
        "WEIGHT: 375 LBS",
        ".",
        "BIO: BORN ON ELTHO III",
        ".",
        "BACKGROUND: THE BOVINARIAN OR",
        "\"MOOMEN\" AS RIMWARD WORLDS CALL",
        "THEM, HAVE BEEN FASCINATED BY THE",
        "GAME FROM THE BEGINNING.  HAVING",
        "PARTICIPATED IN THE INSURRECTION",
        "AT ALPHA PRAM AND THEN IN THE",
        "FAILED COUP AT SARTUS I, ALDUS",
        "KADEN WENT INTO THE ONLY",
        "PROFESSION LEFT TO HIM.",
        "     THE GREEN QUARTER TOOK HIM",
        "IMMEDIATELY AND AFTER ONLY THREE",
        "HUNTS HE WAS TRANSFERED TO THE",
        "RED HUNTER UNDER TOBIAS'S COMMAND.",
},
{
        "MUTANT",
        ".",
        "NAME: SPECIMEN 7",
        ".",
        "RACE: HOMO DEGENEROUS",
        ".",
        "AGE: ?",
        ".",
        "HEIGHT: 5'10'' (WHEN STANDING)",
        ".",
        "WEIGHT: 250 LBS",
        ".",
        "BIO: HUMAN VARIANT ENGINEERED BY",
        "JANEX CORP. GEN-TECH DIVISION.",
        ".",
        "BACKGROUND: AN ERROR IN OXYGEN",
        "FEEDS TO THE BIRTHING TANKS",
        "RESULTED IN ABNORMAL INTELLIGENCE",
        "LEVELS IN HIS STRAIN.  ESCAPING THE",
        "STERILIZATION THAT FOLLOWED",
        "SPECIMEN 7 IS THE ONLY REMAINING",
        "MEMBER OF HIS RACE.",
        "     HE JOINED GREEN QUARTER HUNT",
        "SQUAD WHEN THEY REALIZED HIS",
        "POTENTIAL AS A HUNTER.",
        ".",
        ".",
},
{
        "DOMINATRIX",
        ".",
        "NAME: THEOLA NOM",
        ".",
        "RACE: HOMO MAJESTRIX",
        ".",
        "AGE: 22",
        ".",
        "HEIGHT: 5'11''",
        ".",
        "WEIGHT: 140 LBS",
        ".",
        "BIO: HUMAN VARIANT ENGINEERED BY",
        "JANEX CORP. GEN-TECH DIVISION.",
        ".",
        "BACKGROUND: INITIALLY ENGINEERED",
        "BY JANEX FOR SALE AS A 'HOME",
        "ENTERTAINMENT SYSTEM', THEOLA",
        "FIRST USED HER PREVIOUSLY LATENT",
        "PSYCHIC ABILITIES ON HER FIRST",
        "OWNER.  THEY NEVER FOUND THE BODY.",
        "     SINCE THEN, SHE HAS BEEN A",
        "FUGITIVE FROM CORE LAW.  RECENTLY",
        "SHE HAS SOUGHT REFUGE WITHIN THE",
        "RED HUNTER HUNT SQUAD.",
        ".",
        ".",
}
    };


/**** VARIABLES ****/

byte             colors[768];
boolean          nointro, nextchar;
extern SoundCard SC;
extern boolean   redo;
int              cdr_drivenum;


/**** FUNCTIONS ****/

void loadscreen(char* s)
{
    byte* l;
    int   i;

    i = CA_GetNamedNum(s);
    l = CA_CacheLump(i);
    VI_DrawPic(0, 0, (pic_t*) l);
    CA_FreeLump(i);
    l = CA_CacheLump(i + 1);
    memcpy(colors, l, 768);
    CA_FreeLump(i + 1);
}


boolean CheckTime(int n1, int n2);

void Wait(longint time)
{
    longint t;

    t = timecount + time;
    while (!CheckTime(timecount, t)) { PvmkTimerSim(); PvmkPresent(); }
}


void DoIntroMenu(void);

void ShowPortrait(int n)
{
    char str1[128];
    int  i;

    sprintf(str1, "CHAR%i", n);
    loadscreen(str1);
    VI_FadeIn(0, 256, colors, 48);
    font = font1;
    for (i = 0; i < 27; i++)
    {
        UpdateSound();
        for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
            if (charinfo[n - 1][i][0] != '.')
            {
                printx = 144;
                printy = 19 + 6 * i;
                sprintf(str1, "%s", charinfo[n - 1][i]);
                FN_RawPrint3(str1);
                Wait(2);
            }
        if (activatemenu)
        {
            DoIntroMenu();
            if (quitgame || gameloaded)
                return;
        }
    }
    for (i = 0; i < 50; i++)
    {
        UpdateSound();
        Wait(35);
        if (activatemenu)
        {
            DoIntroMenu();
            if (quitgame || gameloaded)
                return;
        }
        if (nextchar)
        {
            nextchar = false;
            break;
        }
    }
    VI_FadeOut(0, 256, 0, 0, 0, 48);
}


void IntroCommand(void)
{
    if ((keyboard[SC_ESCAPE] || keyboard[SC_ENTER]) && timecount > keyboardDelay)
    {
        activatemenu  = true;
        keyboardDelay = timecount + 20;
    }
    if (keyboard[SC_SPACE] && timecount > keyboardDelay)
    {
        nextchar      = true;
        keyboardDelay = timecount + 20;
    }
}
void IntroStub(void) {}


void DoIntroMenu(void)
{
    byte *temp, oldcolors[768];

    memcpy(oldcolors, colors, 768);
    temp = (byte*) malloc(SCREENBYTES);
    if (temp == NULL)
        MS_Error("DoIntroMenu: No memory for temp screen");
    memcpy(temp, screen, SCREENBYTES);
    memset(screen, 0, SCREENBYTES);
    VI_SetPalette(CA_CacheLump(CA_GetNamedNum("palette")));
    player.timecount = timecount;
    ShowMenu(0);
    if (!quitgame && !gameloaded)
    {
        memset(screen, 0, SCREENBYTES);
        memcpy(colors, oldcolors, 768);
        VI_SetPalette(colors);
        memcpy(screen, temp, SCREENBYTES);
    }
    free(temp);
    timecount    = player.timecount;
    activatemenu = false;
    // lock_region((void near *)IntroCommand,(char *)IntroStub - (char near *)IntroCommand);
    INT_TimerHook(IntroCommand);
    keyboardDelay = timecount + KBDELAY;
}


boolean CheckDemoExit(void)
{
    if (activatemenu)
    {
        activatemenu = false;
        nextchar     = false;
        //   unlock_region((void near *)IntroCommand,(char *)IntroStub - (char near *)IntroCommand);
        //   INT_TimerHook(NULL);
        return true;
    }
    return false;
}


void DemoIntroFlis(char* path);


void MainIntro(void)
{
    char path[64];

    //sprintf(path, "%c:\\MOVIES\\", cdr_drivenum + 'A');
	strcpy(path, "movies/"); //pvmk
    DemoIntroFlis(path);
}


void DemoIntroFlis(char* path)
{
    char name[64];

    fontbasecolor = 0;
    font          = font1;

    sprintf(name, "%sTEXT.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sWARP01.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sWARP02.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sINSHIP01.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sARBITER.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sINSHIP02.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;
    Wait(140);

    sprintf(name, "%sCHAR1.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sCHAR2.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sCHAR3.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sCHAR4.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sCHAR5.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sINSHIP03.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sDROPPOD.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sSHP1.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sCITYBURN.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sRUBBLE.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sTHF1.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sTHF2.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sTHF3.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sTHF4.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sINSHIP04.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sWARP05.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;

    sprintf(name, "%sLOGOFLY.FLI", path);
    playfli(name, 0);
    if (CheckDemoExit())
        return;
    Wait(210);
}


void DemoIntro(void)
{
    int   i;
    FILE* f;

    fontbasecolor = 0;
    font          = font1;

    f = fopen("MOVIES\\TEXT.FLI", "rb");
    if (f != NULL)
    {
        fclose(f);
        DemoIntroFlis("MOVIES\\");
        return;
    }

    VI_FillPalette(0, 0, 0);
    loadscreen("SOFTLOGO");
    VI_FadeIn(0, 256, colors, 48);
    Wait(210);
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;

    loadscreen("C7LOGO");
    VI_FadeIn(0, 256, colors, 48);
    Wait(210);
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;

    loadscreen("LOGO");
    VI_FadeIn(0, 256, colors, 48);
    Wait(210);
    VI_FillPalette(63, 63, 63);
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;

    loadscreen("INTRO00");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("IT IS THE YEAR 15392 DURING THE THIRD AGE OF MAN.\n"
                        "SCAVENGER HUNTS MEAN BIG MONEY FOR THE CRIMINAL\n"
                        "ELITE.  COVERTLY RECRUITED BY AN ENIGMATIC FACTION\n"
                        "OF THE A.V.C. YOU ARE A MEMBER OF THE RED HUNTER\n"
                        "ELITE ACQUISITION SQUAD.");
        Wait(5);
    }
    for (i = 0; i < 70; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO01");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("USING YOUR NEEDLING SHIP, THE RED HUNTER, IT IS YOUR\n"
                        "JOB TO WARP THROUGH TO UNSUSPECTING FRINGE WORLDS...");
        Wait(5);
    }
    for (i = 0; i < 42; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO02");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("...PLUMMET FROM ORBIT IN YOUR DROP SHIP, SUPPRESSING\n"
                        "NONCOOPERATIVE ENTITIES (NOPS) AS BEST YOU CAN IN\n"
                        "ORDER TO ACQUIRE YOUR PRIMARY TARGET ITEMS.");
        Wait(5);
    }
    for (i = 0; i < 42; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO03");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("THE SIZE AND SPEED OF THE DROP SHIP SHOULD BE SUFFICIENT\n"
                        "TO DRIVE NONCOOPERATIVES FROM THE DROP ZONE UPON IMPACT.");
        Wait(5);
    }
    for (i = 0; i < 42; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FillPalette(0, 0, 0);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO04");
    VI_SetPalette(colors);
    for (i = 0; i < 21; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FadeOut(0, 256, 64, 64, 64, 48);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO05");
    VI_FadeIn(0, 256, colors, 48);
    for (i = 0; i < 21; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO06");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("IN SHORT, IF IT'S MOVING AROUND, KILL IT...");
        Wait(5);
    }
    for (i = 0; i < 28; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;


    loadscreen("INTRO07");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("...AND IF IT'S NOT NAILED DOWN, STEAL IT.");
        Wait(5);
    }
    for (i = 0; i < 28; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;

    loadscreen("INTRO08");
    VI_FadeIn(0, 256, colors, 48);
    for (fontbasecolor = 0; fontbasecolor < 9; ++fontbasecolor)
    {
        printy = 160;
        FN_CenterPrintf("YOU'RE A SCAVENGER.  YOU'VE GOT A GUN.\n"
                        "LET'S GET SOME.");
        Wait(5);
    }
    for (i = 0; i < 28; i++)
    {
        Wait(10);
        if (CheckDemoExit())
            return;
    }
    if (CheckDemoExit())
        return;
    VI_FadeOut(0, 256, 0, 0, 0, 48);
    if (CheckDemoExit())
        return;
}


void intro(void)
{
    int i;

    //lock_region((void near*) IntroCommand, (char*) IntroStub - (char near*) IntroCommand);
    INT_TimerHook(IntroCommand);

    PlaySong("INTRO.S3M", 0);

    VI_FillPalette(0, 0, 0);

    quitgame   = 0;
    gameloaded = 0;

#ifdef DEMO
    DemoIntro();
#else
    if (!redo)
        MainIntro();
#endif

    redo = 0;

    // lock_region((void near *)IntroCommand,(char *)IntroStub - (char near *)IntroCommand);
    // INT_TimerHook(IntroCommand);
    VI_FillPalette(0, 0, 0);
    activatemenu = false;
    for (;;)
    {
        for (i = 1; i < 6; i++)
        {
            ShowPortrait(i);
            if (quitgame || gameloaded)
            {
                //unlock_region((void near*) IntroCommand,
                //              (char*) IntroStub - (char near*) IntroCommand);
                INT_TimerHook(NULL);
                return;
            }
        }
    }
}


void LoadMiscData(void)
{
    int i;

    printf(".");
    font1 = CA_CacheLump(CA_GetNamedNum("FONT1"));
    printf(".");
    font2 = CA_CacheLump(CA_GetNamedNum("FONT2"));
    printf(".");
    font3        = CA_CacheLump(CA_GetNamedNum("FONT3"));
    statusbar[0] = CA_CacheLump(CA_GetNamedNum("STATBAR1"));
    statusbar[1] = CA_CacheLump(CA_GetNamedNum("STATBAR2"));
    statusbar[2] = CA_CacheLump(CA_GetNamedNum("STATBAR3"));
    statusbar[3] = CA_CacheLump(CA_GetNamedNum("STATBAR4"));
    printf(".");
    for (i = 0; i < 10; i++)
        heart[i] = CA_CacheLump(CA_GetNamedNum("HEART") + i);
    printf(".");
    backdrop = malloc(256 * 256);
    if (backdrop == NULL)
        MS_Error("Out of memory for BackDrop");
    printf(".");
    for (i = 0; i < 256; i++)
        backdroplookup[i] = (byte*) backdrop + 256 * i;
    printf(".");
}


void checkexit()
{
    if (newascii && lastascii == 27)
        MS_Error("Cancel Startup");
    newascii = false;
}



void LoadData(void)
{
    boolean netplay = false;
    int     parm;
    longint netaddr;

    if (MS_CheckParm("nointro"))
        nointro = true;
    if (MS_CheckParm("record"))
        recording = true;
    if (MS_CheckParm("playback"))
        playback = true;
    if (MS_CheckParm("debugmode"))
        debugmode = true;
    parm = MS_CheckParm("net");
    if (parm && parm < my_argc - 1)
    {
        netaddr = atoi(my_argv[parm + 1]);
        netplay = true;
        netmode = 1;
    }

    if (MS_CheckParm("nospawn"))
        nospawn = true;
    if (MS_CheckParm("ticker"))
        ticker = true;
    CA_InitFile("GREED.BLO");
    if (netplay)
    {
        NetInit((void*) netaddr);
        nointro = true;
    }
    InitSound();
    if (SC.vrhelmet == 1)
        printf("VRHelmet: SimulEyes VR\n");
    INT_Setup();
    checkexit();
    RF_PreloadGraphics();
    checkexit();
    RF_Startup();
    checkexit();
    LoadMiscData();
    checkexit();
    VI_Init(0);
}



int main(int argc, char* argv[])
{
    my_argc = argc;
    my_argv = argv;
    setbuf(stdout, NULL);
#ifdef DEMO
    printf("\nIn Pursuit of Greed DEMO (v%s)\n\n", DEMOVERSION);
#else
    printf("\nIn Pursuit of Greed (v%s)\n\n", VERSION);
#endif
    LoadData();
restart:
    if (nointro)
    {
        redo = false;
        VI_SetPalette(CA_CacheLump(CA_GetNamedNum("palette")));
        if (netmode)
            newplayer(SC.netmap, 0, SC.netdifficulty);
        else
            newplayer(0, 0, 2);
        maingame();
    }
    else
        intro();
    if (!quitgame)
    {
        maingame();
        if (redo)
            goto restart;
    }
    else
        StopMusic();
    INT_Shutdown();
    VI_SetTextMode();
#ifdef DEMO
    printf("                           In Pursuit of GREED\n"
           "                 THIS IS A FREE DEMO.  DISTRIBUTE FREELY.\n"
           "               So... you played the demo.  You want the game.\n\n"
           "* Rampage in 22 levels guaranteed to satisfy your agression obsession!\n"
           "* Humiliate your opponents as you crush them on 10 death-match levels.\n"
           "* Obliterate three complete worlds -- the Desarian Orbital Penal Colony, the\n"
           "  City-Temple of Ristanak, and the Mt. Kaal Jump Base.\n"
           "* Devistate your enemies with powerful new items such as the Holosuit,\n"
           "  Warp Jammer, and Soul Stealer!\n\n"
           "The full game is scheduled to ship in February, so order your copy now\n"
           "for just $39.95 plus $5.00 shipping. Call 1-800-831-2694 or 1-318-221-8718 to\n"
           "order by phone with a credit card (see ORDER.TXT for more information).  Or\n"
           "mail a check or money order (in U.S. funds) to:\n"
           "SOFTDISK PUBLISHING            *****************SAVE $5.00!******************\n"
           "Greed Demo Offer #GDC115       *     Complete the demo and you'll learn     *\n"
           "P.O. Box 30008                 *   a 'secret phrase' worth FREE shipping!   *\n"
           "Shreveport, LA 71130-0008      **********************************************\n"
           "              Visit our Internet site: http://www.softdisk.com\n");

    //       "                           You have two options.\n"
    //        "1) You could copy the game from your friend who already has the registered\n"
    //        "version or pirate it from the Net, but let me tell you something first.\n\n"
    //        "For just pennies a day you could help feed and clothe a CHANNEL 7 employee.\n"
    //        "The folks here at CHANNEL 7 have gone for six months without eating or\n"
    //        "sleeping, working on a shoe-string budget to bring you this quality game.\n"
    //        "If you don't buy it, Rob will have to get a real job, Jeremy will have to go\n"
    //        "back to school, Jeff, Brady, and Chris will be forced to go back to government\n"
    //        "jobs and Tiny Tim won't get his Christmas turkey.\n"
    //        "				    OR\n"
    //        "2) You could buy the game and rest easy knowing you've put food on the table\n"
    //        "for a handful of starving artists.\n\n"
    //        "		       Do the right thing. Call now.\n"
    //        "       Within USA call 1-800-831-2694 otherwise call 1-318-221-8718\n"
    //        " No Phone? Write us at: Softdisk, PO BOX 30008, Shreveport, LA 71130-0008 USA\n"
    //        "            Get it for just $39.95 + $5 Shipping and Handling.\n"
    //        "         GREED order number:   3.5'' Disks 0134B7     CDROM 0001B2");

#else
    printf("In Pursuit of Greed\n"
           "Developed by Mind Shear Software\n"
           "In Pursuit of Greed is a trademark of Softdisk, Inc.\n"
           "Copyright (c) 1996 Softdisk, Inc.\n"
           "All rights reserved.\n\n");
    if (MS_CheckParm("frames") && frames)
        printf("\n%f fps\n", (float) (frames * 70) / (float) player.timecount);
#endif
    //while (kbhit())
    //    getch();
    goto restart;
}
