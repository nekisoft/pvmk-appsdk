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

#include "d_global.h"
#include "protos.h"

/**** CONSTANTS ****/

#define PLAYERMOVESPEED FRACUNIT * 2.5
#define PLAYERTURNSPEED 8
#define MOVEUNIT FRACUNIT / 2
#define TURNUNIT 2
#define FALLUNIT FRACUNIT

weapon_t weapons[19] =  // weapon info
    {
        // chargerate charge chargetime ammotype ammorate
        {6,  0, 0, 0, 0, },  // 0  double shot rifle    energy      // removed!
        {4,  0, 0, 1, 1, },  // 1  psyborg #2           ballistic
        {3,  0, 0, 1, 1, },  // 2  pulserifle           ballistic
        {1,  0, 0, 2, 1, },  // 3  flamer               plasma
        {12, 0, 0, 0, 5, },  // 4  spreadgun            energy
        {32, 0, 0, 2, 5, },  // 5  bfg                  plasma      // removed!
        {32, 0, 0, 3, 5, },  // 6  grenade              grenade     // removed!
        {6,  0, 0, 0, 0, },  // 7  psyborg #1           energy
        {12, 0, 0, 0, 0, },  // 8  lizard #1            energy
        {8,  0, 0, 1, 2, },  // 9  lizard #2            ballistic
        {6,  0, 0, 0, 1, },  // 10 specimen #2          energy
        {6,  0, 0, 1, 1, },  // 11 mooman #2            ballistic
        {12, 0, 0, 2, 2, },  // 12 trix #2              plasma
        {12, 0, 0, 0, 0, },  // 13 mooman #1            energy
        {12, 0, 0, 0, 0, },  // 14 specimen #1          energy
        {12, 0, 0, 0, 0, },  // 15 trix #1              energy
        {3,  0, 0, 0, 1, },  // 16 red gun              energy
        {12, 0, 0, 1, 7, },  // 17 blue gun             ballistic
        {20, 0, 0, 2, 75,},  // 18 green gun            plasma
    };

/*
 0 energy
 1 ballistic
 2 plasma
 3 grenade
*/

byte weaponstate[19][5] =  // finite state machine (modes of weapon frames)
    {
        {0, 0, 0, 0, 0, }, // 0
        {0, 2, 0, 0, 0, }, // 1
        {0, 2, 3, 0, 0, }, // 2
        {0, 2, 3, 0, 0, }, // 3
        {0, 2, 3, 0, 0, }, // 4
        {0, 2, 3, 4, 0, }, // 5
        {0, 0, 0, 0, 0, }, // 6
        {0, 2, 0, 0, 0, }, // 7
        {0, 2, 3, 0, 0, }, // 8
        {0, 2, 0, 0, 0, }, // 9
        {0, 2, 3, 0, 0, }, // 10
        {0, 2, 0, 0, 0, }, // 11
        {0, 2, 0, 0, 0, }, // 12
        {0, 2, 0, 0, 0, }, // 13
        {0, 2, 0, 0, 0, }, // 14
        {0, 2, 0, 0, 0, }, // 15
        {0, 0, 0, 0, 0, }, // 16
        {0, 2, 0, 0, 0, }, // 17
        {0, 2, 3, 4, 0, }, // 18
    };


int headmove[MAXBOBS] =  // sinusoidal head movement table
    {
        // cos(a-90) cos(a-90)*65536
        BOBFACTOR * 0,
        BOBFACTOR * 6813,   // .2079     13625,
        BOBFACTOR * 13327,  // .4067     26653,
        BOBFACTOR * 19261,  // .5878     38522,
        BOBFACTOR * 24350,  // .7431     48700,
        BOBFACTOR * 28377,  // .8660     56754,
        BOBFACTOR * 31166,  // .9511     62331,
        BOBFACTOR * 32588,  // .9945     65176,
        BOBFACTOR * 32588,  // .9945     65176,
        BOBFACTOR * 31166,  // .9511     62331,
        BOBFACTOR * 28377,  // .8660     56754,
        BOBFACTOR * 24350,  // .7431     48700,
        BOBFACTOR * 19261,  // .5878     38522,
        BOBFACTOR * 13327,  // .4067     26653,
        BOBFACTOR * 6813,   // .2079     13625,
        BOBFACTOR * 0,
        BOBFACTOR * -6813,   // .2079     13625,
        BOBFACTOR * -13327,  // .4067     26653,
        BOBFACTOR * -19261,  // .5878     38522,
        BOBFACTOR * -24350,  // .7431     48700,
        BOBFACTOR * -28377,  // .8660     56754,
        BOBFACTOR * -31166,  // .9511     62331,
        BOBFACTOR * -32588,  // .9945     65176,
        BOBFACTOR * -32588,  // .9945     65176,
        BOBFACTOR * -31166,  // .9511     62331,
        BOBFACTOR * -28377,  // .8660     56754,
        BOBFACTOR * -24350,  // .7431     48700,
        BOBFACTOR * -19261,  // .5878     38522,
        BOBFACTOR * -13327,  // .4067     26653,
        BOBFACTOR * -6813,   // .2079     13625,
    };

int weapmove[MAXBOBS] = {
    WEAPFACTOR * 0,      WEAPFACTOR * 6813,   WEAPFACTOR * 13327,  WEAPFACTOR * 19261,
    WEAPFACTOR * 24350,  WEAPFACTOR * 28377,  WEAPFACTOR * 31166,  WEAPFACTOR * 32588,
    WEAPFACTOR * 32588,  WEAPFACTOR * 31166,  WEAPFACTOR * 28377,  WEAPFACTOR * 24350,
    WEAPFACTOR * 19261,  WEAPFACTOR * 13327,  WEAPFACTOR * 6813,   WEAPFACTOR * 0,
    WEAPFACTOR * -6813,  WEAPFACTOR * -13327, WEAPFACTOR * -19261, WEAPFACTOR * -24350,
    WEAPFACTOR * -28377, WEAPFACTOR * -31166, WEAPFACTOR * -32588, WEAPFACTOR * -32588,
    WEAPFACTOR * -31166, WEAPFACTOR * -28377, WEAPFACTOR * -24350, WEAPFACTOR * -19261,
    WEAPFACTOR * -13327, WEAPFACTOR * -6813,
};


char* randnames[MAXRANDOMITEMS] = { "Quantum Energy Lattice",
                                    "Verton Battery Pack",
                                    "Hezfu Mind Grubs",
                                    "Solar Particle Collector",
                                    "Exo-Suit Patching Kit",
                                    "Gates22 Anti-Viral Algorithms",
                                    "Pelermid Gorgon Scale",
                                    "Fertility Charm from Vozara 3",
                                    "D&H Nucleo-Pistol (disfunct)",
                                    "Needle Drive Micro-Insulation",
                                    "Formani Shunt Cortex",
                                    "Auto-Med Surgical Platform",
                                    "Gygaxian Meditation Tome",
                                    "Pan Flute of Harask",
                                    "Cryo-Fugue Refrigerant GelPak",
                                    "Veros VIII Crown Jewels",
                                    "Adhesive Message Pads",
                                    "Self-Replicating Food Ration",
                                    "Selukani Hull Scouring Fungi",
                                    "Mnemony 6 Neural Net Crystals",
                                    "Harag Species Bio-Index",
                                    "Dane-Kyna Seeker Module",
                                    "Inertia-Absorb Armor Plating",
                                    "Selan Energy Scythe",
                                    "Transcendant Flea",
                                    "Xolas-Prime Prayer Icon",
                                    "Kriijing Spider Mono-Webbing",
                                    "Dysolv-It Pressurized Spray",
                                    "Galactic Shock Troop Insignia",
                                    "Hawking Singularity Framework",
                                    "Enviro-Stabilizer",
                                    "Audio Signal Generator",
                                    "EchoDrome Motion Sensor",
                                    "RGK Heat Sensor",
                                    "Rad12 Radiation Patches",
                                    "Hydro Nutrient Solu-Drink",
                                    "Semantik Lingua Pad",
                                    "Werton System Travel Guide",
                                    "Kaltrop mk 4 Mine (nonfunct)",
                                    "Jabberwock Phase Dish",
                                    "D-Gauss Energy Dampener",
                                    "Vyvald Amino Acid Solution",
                                    "Zheldisian Logic Trap",
                                    "Feldscape Holo-Generator",
                                    "Ruwelda-Trieu Currency",
                                    "Android Recharge Station",
                                    "Vesppil Witch Charm",
                                    "Iedine Crystal Psyche Skull" };

char* pickupmsg[]
    = { "Grenades picked up!",    "ReversoPill picked up!",  "Proximity Mines picked up!",
        "Time Bombs picked up!",  "Decoy picked up!",        "InstaWall picked up!",
        "Clone picked up!",       "HoloSuit picked up!",     "Invisibility Shield picked up!",
        "Warp Jammer picked up!", "Soul Stealer picked up!", "Ammo Box picked up!",
        "Auto-Doc picked up!",    "Utility Chest picked up!" };

char pickupamounts[] = { 5, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 125 };

char* pickupammomsg[] = {
    "Energy ammo picked up!",
    "Bullets picked up!",
    "Plasma ammo picked up!",
};


char* missioninfo[30][3] = {
	{
    "",
    "",
    "",
	},
#ifndef DEMO

    /*"Sublevel 1:\n"
    "Inmate Processing Area\n"
    " THIS MISSION WILL BEGIN IN THE INMATE PROCESSING AREA.  YOUR\n",
    "PRIMARY TARGET IS AN EXPERIMENTAL EXPLOSIVE HIDDEN IN THIS\n"
    "SUBLEVEL.  SECONDARY GOALS ARE PHOSPHER PELLETS AND DELOUSING\n",
    "KITS USED TO CLEANSE THE VILE URCHINS AS THEY ARRIVE ON STATION.\n"
    "SNATCH THE PRIMARY ITEM ALONG WITH A 60,000 POINT QUOTA AND WE'LL\n"
    "OPEN A TRANSLATION NEXUS TO THE NEXT SUBLEVEL.\n", */
{
    "Sublevel 2:\n"
    "Primary Holding\n"
    " THIS SUBLEVEL SERVES AS THE SUPPLIES ENTRY POINT ON THE COLONY\n"
    "AS WELL AS THE INMATE FREEHOLD.  THE PRIMARY OBJECT TO BE\n"
    "ACQUIRED IN THIS SUBLEVEL IS A SPACE GENERATOR WHICH HELPS POWER\n",
    "THE OUTER SHIELDING FOR THE DETENTION FACILITY.  REMOVING THIS\n"
    "ITEM IS NECESSARY IF YOU ARE TO MOVE INTO THE NEXT SUBLEVEL.\n"
    " SECONDARY TARGETS INCLUDE VIALS OF TRUTH SERUM AND HYPODERMIC\n"
    "NEEDLES USED DURING INMATE INTERROGATION.  ONCE YOU'VE ACQUIRED\n",
    "THE PRIMARY GOAL AND YOUR POINT TOTAL MEETS OR EXCEEDS 55,000\n"
    "WE'LL OPEN A TRANSLATION NEXUS TO THE NEXT SUBLEVEL.  OH, AND ONE\n"
    "LAST THING...\n"
    "...DON'T TRIGGER THE AIRLOCKS IN THIS AREA UNLESS YOU WANT THE\n"
    "AIR RIPPED VIOLENTY FROM YOUR LUNGS.\n",
},
{
    "Sublevel 3:\n"
    "Detention Facility\n"
    "THE RIOT IN THE COLONY HAS REACHED NEW HEIGHTS OF CHAOS.  THE\n",
    "WARDEN'S QUARTERS HAVE BEEN BREACHED, HIS HEAD WAS CUT FROM HIS\n"
    "BODY AND SUBSEQUENTLY HIDDEN IN A JAR ON THIS SUBLEVEL.  THE\n"
    "A.V.C. IS INTERESTED IN REVIVING THE BRAIN SO IT HAS BECOME YOUR\n",
    "PRIMARY TARGET ITEM.  SECONDARIES INCLUDE LUBRICANTS AND\n"
    "EMERGENCY LANTERNS.  IF AFTER ACQUIRING THE PRIMARY ITEM YOUR\n"
    "POINT TOTAL MEETS OR EXCEEDS 60,000 POINTS WE'LL OPEN A NEXUS\n"
    "AND TRANSLATE YOU TO THE PURIFICATION FACILITY.\n"
    "HAPPY HEAD HUNTING.\n",
},
{
    "Sublevel 4:\n"
    "Station Manufacturing Facility\n"
    "THIS NEXT AREA IS THE ON-STATION PRODUCTION CENTER USED BY THE\n",
    "PRISONERS.  THIS WILL BE THE FIRST TEST OF YOUR TRUE SCAVENGING\n"
    "ABILITIES.  INSTEAD OF HAVING PRIMARY OR SECONDARY OBJECTS, YOU\n"
    "WILL HAVE TO REACH A SCORE OF 50000 BY FINDING RANDOM BONUS\n"
    "ITEMS AND KILLING NOPS.  THE KEYS TO SUCCEEDING HERE ARE SPEED,\n",
    "ACCURACY, AND CONSERVATION OF YOUR RESOURCES.\n",
},
{
    "Sublevel 5:\n"
    "Water Purification Facility\n"
    "THE PRISON BILGE WAS OF NO INTEREST TO THE A.V.C. WHEN THE HUNT\n"
    "BEGAN, BUT AFTER ACCESSING THE ON-STATION COMPUTER WE FOUND\n"
    "STRANGE TRACE COMPOUNDS IN THE WATER TANKS.  THE UNIQUE CHEMICAL\n",
    "SIGNATURE IS THAT OF A PHLEGMATIC EEL--A RARE FRESH WATER\n"
    "CREATURE OF NON-GENE CLUSTER-VIRUS COMPOSITION.  IT IS A WORTHY\n"
    "FIND AND OF GREAT INTEREST TO A.V.C. RESEARCH.\n",
    " YOU ARE TO DRAIN THE PRISON WATER RESEVOIRS TO ACQUIRE THE EEL.\n"
    "SECONDARIES INCLUDE WATER AND OXYGEN TANKS.  THE POINT QUOTA FOR\n"
    "EXITING HAS BEEN SET AT 70,000.\n"
    " WE ARE WITH YOU.\n",
},
{
    "Sublevel 6:\n"
    "Power Substation Alpha\n"
    " THERE IS A POWER COUPLING THAT MAINTAINS A CYRO-RIDON FORCE\n",
    "FIELD THAT PREVENTS YOUR TRANSLATION INTO THE COMMAND CENTER. \n"
    "YOUR PRIMARY TARGET IS THE COUPLING ITSELF.  YOUR SECONDARY\n"
    "TARGETS ARE RAD-SHIELD GOGGLES AND VERIMAX INSULATED GLOVES\n"
    "LOCATED WITHIN THE POWER CHAMBERS AND STORAGE ROOMS.\n"
    " IF YOU ACQUIRE THE COUPLING AND ACHIEVE A QUOTA OF 80,000\n",
    "WE'LL MOVE YOU THROUGH TO THE NEXT SUBLEVEL.\n"
    "\n",
},
{
    "Sublevel 7:\n"
    "Maximum Security Detention Area\n"
    " THIS HUNT IS ALMOST COMPLETE.  WE WOULD HAVE MOVED YOU ON INTO\n"
    "THE COMMAND CENTER BUT THE MISSION ARBITER DISCOVERED THAT THE\n"
    "ENTIRE POPULATION OF THE STATION HAD BEEN GENE-CODED AND THAT\n",
    "THE GENE CODING CUBE IS HIDDEN IN THIS AREA.  THE A.V.C.\n"
    "IS INTERESTED IN OBTAINING THE SAMPLES FOR MILITARY RESEARCH SO\n"
    "WE'RE SENDING YOU IN.  ACQUIRE THE GENE CODING CUBE AND A\n",
    "POINT QUOTA OF 90,000 AND YOU EARN TRANSLATION TO THE FINAL\n"
    "SUBLEVEL.  PICK UP TRIBOLEK CUBES AND SPACE HEATERS AS\n"
    "SECONDARIES TO HELP REACH THE QUOTA.\n",
},
{
    "Sublevel 8:\n"
    "Primary Command Center\n"
    " YOU HAVE ARRIVED.  THE BRASS RING OF BYZANT IS SOMEWHERE IN THE\n"
    "COMMAND CENTER.  IT IS UP TO YOU TO FIND IT AND BRING IT BACK.\n"
    "THERE IS A POWERFUL SHIELD AROUND THE RING, CONTROLLED BY THREE\n"
    "NEARBY POWER STATIONS.  EVEN MORE, EACH POWER STATION HAS ITS\n",
    "OWN SHIELD THAT IS CONTROLLED BY A SENTINAL GUARD.  SUMMON EACH\n"
    "SENTINAL AND DESTROY IT.  BY DESTROYING THE SENTINALS, YOU SHOULD\n"
    "DESTROY THE SHIELDS TO THE POWER STATIONS AS WELL.  STAND ON THE\n"
    "POWER CORES TO DISABLE THEM.  WHEN YOU FINALLY MAKE YOUR WAY TO\n",
    "THE RING, BE PREPARED TO LEAVE IN A HURRY.  THERE IS STILL A\n"
    "QUOTA OF 150,000 POINTS, SO REACTOR COOLANT CONTAINERS AND\n"
    "POWER FLOW CALIBRATORS HAVE BEEN DESIGNATED AS SECONDARY POINT\n"
    "ITEMS.  YOUR DATE WITH TRUE GLORY AWAITS...DON'T BE LATE.\n",
},
{
    /* TEMPLE */

    "Sublevel 1:\n"
    "Outer Towers\n"
    "THIS WILL BE YOUR ENTRY POINT FOR THE CITY-TEMPLE.  YOUR FIRST\n"
    "TARGET WILL BE THE MOST HOLY INCANTATION BRAZIER.  BEFORE YOU CAN\n",
    "GET TO IT YOU'LL NEED TO BRING DOWN THE BARRIER GUARDING IT USING\n"
    "THE SWITCHES LOCATED IN THE TOWERS THROUGHOUT THE SUBLEVEL.\n",
    "SECONDARY ITEMS WILL BE RITUAL CANDLES HIDDEN IN THE NOOKS AND\n"
    "CRANNIES OF THE TOWER.  ACQUIRE THE PRIMARY TARGET AND A POINT\n"
    "QUOTA OF 50,000 AND ON YOU'LL GO.\n",
},
{
    "Sublevel 2:\n"
    "Ritual Spires\n"
    "YOUR FIRST OBSTACLE FOR THIS AREA WILL BE GETTING BEYOND THE\n"
    "OUTER WALL INTO THE COURTYARD.  ONCE INSIDE YOU HAVE TO FIND A\n"
    "WAY INTO THE SPIRES THEMSELVES.  THERE ARE THREE SMALLER SPIRES\n",
    "AND ONE LARGER ONE WHICH HOUSES YOUR PRIMARY TARGET, THE IDOL OF\n"
    "THE FELASHA PONT.  SECONDARY ITEMS WILL BE PRAYER SCROLLS ON\n"
    "WHICH ARE WRITTEN SPECIAL IDTH RITUAL MANTRA'S.  THE A.V.C WANTS\n"
    "THE ICON AND SCROLLS TO RANSOM BACK TO THE IDTH FROM WHICH THEY\n",
    "WERE TAKEN.\n"
    " ACQUIRE THE PRIMARY AND A POINT QUOTA OF 55,000 AND WE'LL\n"
    "TRANSLATE YOU OUT.\n",
},
{
    "Sublevel 3:\n"
    "Temple Catacombs\n"
    "YOUR PRIMARY GOAL FOR THIS AREA IS THE BOOK OF CHANTS SINCE THE\n"
    "BOOK WAS CONSTRUCTED BY THE ANCIENT TRIBE-OF-NINE. ITS VALUE IS\n"
    "OBVIOUS.\n",
    " SCANS OF THE CATACOMBS HAVE ALSO REVEALED THE PRESENCE OF A\n"
    "PRICELESS RARE INSECT KNOWN AS THE SILVER BEETLE.  BECAUSE OF\n"
    "THEIR WORTH THEY WILL MAKE EXCELLENT SECONDARY TARGETS.\n",
    " AFTER YOU HAVE ACQUIRED THE PRIMARY OBJECT AND YOUR POINT TOTAL\n"
    "MEETS OR EXCEEDS 60,000 WE'LL... EH... YOU KNOW THE ROUTINE.\n",
},
{
    "Sublevel 4:\n"
    "Training Grounds\n"
    " AS WELL AS PREPARING THEMSLEVES PSYCHICALLY, THE ACOLYTES OF THE\n"
    "AKI-VORTELASH ORDER ARE GIVEN TO THE CARE AND TRAINING OF WAR\n"
    "SLUGS AS PART OF THEIR DISCIPLINE.  IT IS FOR THIS REASON THAT\n"
    "THE PRIESTHOOD COMBINED THE SLUG LARVA HOLDS AND THEIR MARTIAL\n",
    "TRAINING GROUNDS.  SECONDARY OBJECTS ARE THEREFORE WAR SLUG\n"
    "LARVAE AND SLUG FOOD.\n"
    "THE PRIMARY TARGET, HOWEVER, IS THE YRKTAREL'S\n"
    "SKULL SCEPTER.  YOU MUST KILL ALL OF THE HIGH PRIESTS IN THE OUTER\n"
    "COURTYARD IN ORDER TO ANGER YRKTAREL, CAUSING HIM TO APPEAR.\n",
    "YOU MUST WREST THE STAFF FROM HIM.  IF YOU DO SO SUCCESSFULLY AND\n"
    "YOUR POINT TOTAL MEETS OR EXCEEDS 65,000, A TRANSLATION NEXUS\n"
    "WILL BE OPENED TO SEND YOU TO THE NEXT SUBLEVEL.\n"
    "GOD SPEED.\n",
},
{
    "Sublevel 5:\n"
    "Summoning Circles\n"
    " DON'T THINK Y'ARK TAREL IS GONE--JUST MAD AND RESTING, SO DON'T\n"
    "DISTURB HIM OR YOU'LL PROBABLY LEARN A NEW MEANING OF HURT.\n"
    " IN THIS AREA YOU WILL FIND 4 SUMMONING CHAMBERS WHICH THE\n"
    "PRIESTHOOD USES TO CONTROL VOID-FORCE AND THE IDTH DEMONS THEY\n"
    "SUMMON.  EACH CHAMBER IS PROTECTED BY A FORCE BARRIER.  THESE\n",
    "BARRIERS CAN ONLY BE PIERCED BY SPECIFIC FORCE KEYS.  YOU MUST\n"
    "FIND THE FOUR KEYS TO THE SUMMONING CHAMBERS, FOR ONLY THEN WILL\n"
    "THE FORCE BARRIERS FALL AND ALLOW YOU INTO THE CHAMBERS.  THE\n"
    "ACTUAL DESECRATION OF THE SUMMONING CIRCLES IS YOUR PRIMARY\n",
    "OBJECTIVE.  THIS IS DONE MERELY BY THOROUGHLY STOMPING ON THEM.\n"
    "ONCE THIS IS DONE, IF YOUR POINT TOTAL MEETS OR EXCEEDS 70,000\n"
    "POINTS WE'LL MOVE YOU TO THE NEXT SUBLEVEL.\n",
},
{
    "Sublevel 6:\n"
    "Priest Village\n"
    "FOR THIS MISSION YOU WILL HUNT FOR THE SACRIFICIAL DAGGER OF\n"
    "SYDRUS.  IT IS HIDDEN IN AN OUTDOOR OFFERING SHRINE AND\n",
    "CAN ONLY BE REACHED AFTER YOU OPERATE AN INTRICATE SERIES OF\n"
    "GUARD SWITCHES.  ALL OF THESE ARE LOCATED IN THE PRIEST QUARTERS,\n"
    "SAVE FOR ONE WHICH WAS PLACED NEAR THE WATER ACCESS NEAR WHAT\n",
    "WAS ONCE THE WAR SLUG KENNEL. SECONDARY GOAL ITEMS ARE CURED\n"
    "FINGER BONES AND PRIEST PAIN ANKHS.  MEET A POINT QUOTA OF\n"
    "85,000 AND SNATCH THE DAGGER AND WE'LL TRANSLATE YOU ONWARD.\n",
},
{
    "Sublevel 7:\n"
    "Vaults of Vortelash\n"
    " THIS IS THE AQUEDUCT MAIN FOR THE ENTIRE CITY-TEMPLE.  A RIVER\n"
    "RUNS THROUGH IT.\n"
    " THIS AREA CONTAINS THREE VAULTS, EACH HOLDING PRIZE GOLD\n",
    "INGOTS, ALL OF WHICH WILL BE YOUR SECONDARY TARGET ITEMS.  YOUR\n"
    "MAIN CONCERN, HOWEVER, IS THE SACRED COW KEPT WITHIN THE LARGEST\n"
    "OF THE VAULTS.  SNARE THE COW AND A POINT TOTAL OF 100,000\n",
    "AND WE'LL TRANSLATE YOU OUT.\n",
},
{
    "Sublevel 8:\n"
    "Inner Sanctum\n"
    " THIS IS IT.  THE A.V.C. SEEKS TO PROFIT FROM A DESTABILIZATION\n"
    "OF THE LOCAL SYSTEM POLITICS.  IN ORDER TO DO THIS IT IS YOUR\n",
    "MISSION TO ERADICATE THE SOUL OF THE PAGAN GOD THE PRIESTHOOD\n"
    "WORSHIPS.  IN ORDER TO DO SO YOU MUST GATHER THE FOUR SOUL ORBS\n"
    "AND BRING THEM TO THE STATUE FROM WHICH HE DRAWS HIS POWER.  BY\n",
    "DOING SO YOU WILL SUMMON HIS TRUE SPIRIT, THAT IT MIGHT BE\n"
    "SLAIN...\n"
    "...PERMANANTLY.\n",

},
{
    /* KAAL */

    "Sublevel 1:\n"
    "Reception Area\n"
    " BEFORE YOU CAN ENTER THE BASE ITSELF YOU MUST PASS THROUGH THE\n"
    "ENTRANCE AT THE FOOT OF THE MOUNTAIN.  PASS THE ENTRANCE GATE\n",
    "BETWEEN THE GLASS ENLOSURES AND THE INNER RECEPTION AREA. YOUR\n"
    "PRIMARY TARGET IS THE QUAI MUMMIFICATION GLYPH WHICH IS A\n"
    "VALUABLE PIECE OF ART FROM THE UNITY PERIOD. IT IS KEPT ATOP A\n"
    "COLUMN BEYOND THE RECEPTION AREA AND SHOULD BE EASY TO LOCATE.\n",
    "TO LOWER THE PILLER, A SERIES OF SECURITY SWITCHES MUST BE SET,\n"
    "ONE OF WHICH IS IN THE SECURITY TOWER (IT'S UP TO YOU TO GET\n"
    "INSIDE.)\n",
},
{
    "Sublevel 2:\n"
    "Primary Resource Hold Zeta\n"
    " WATCH YOUR BACK.  THIS AREA IS A MAZE OF CRATES AND SENTRY\n"
    "SPHERES ARE EVERYWHERE.  YOUR PRIMARY GOAL IS A SHIPMENT OF VIRAL\n",
    "STABALIZATION PODS, WHILE THE SECONDARY TARGET ITEMS ARE THE\n"
    "ACCOMPANYING DENATURED BIO-PROTEINS TO BE USED WITH THE VIRAL\n",
    "PODS.  PROTEIN CONTAINERS ABOUND.  POINTS SHOULD BE NO PROBLEM\n"
    "HERE.  WHEN YOU'VE AQUIRED AT LEAST ONE PRIMARY AND A POINT QUOTA\n"
    "OF 70,000 WE'LL MOVE YOU TO THE NEXT AREA.\n",
},
{
    "Sublevel 3:\n"
    "The Hanger\n"
    " THE A.V.C. HAS ACQUIRED INFORMATION WHICH SUGGESTS SECRET\n"
    "EXPERIMENTS WERE SECRETLY CONDUCTED ON THE JUMP BASE INVOLVING\n"
    "VOID-MATRIX TRANSLATION.  YOU ARE LOOKING FOR THE FISSURE-PRISM\n",
    "THEY HAD TO BE USING TO GENERATE THE TROJAN POINTS NECESSARY FOR\n"
    "SUCH EXPERIMENTS. SINCE THEY WOULDN'T HAVE HAD TIME TO MOVE IT\n"
    "FAR FROM THE JUNCTION IT MUST BE LOCATED SOMEWHERE IN THE HANGER.\n"
    "THE JUMP TROOPS KNOW YOU'RE COMING AND MAY HAVE HIDDEN. IF SO IT\n",
    "COULD BE WELL GUARDED.\n"
    " SECONDARY TARGETS ARE SHUNT MATRICES AND PLASMA COUPLINGS, THE\n"
    "OTHER TWO COMPONENTS FOR A VOID-TRANSLATION DEVICE.\n"
    " GET YOURSELF 90,000 IN POINTS AND THE PRIMARY AND YOU'RE OUT\n"
    "OF THERE.  GOOD LUCK.\n",
},
{
    "Sublevel 4:\n"
    "Cybergenation Facility\n"
    " IN THIS AREA YOU MUST ACQUIRE THE PSIFLEX DATA CUBE WHICH\n"
    "CONTAINS THE BIRTHING HISTORIES AND GENETIC SIGNATURES FOR HALF\n",
    "OF THE MILLION STRATEGIC SUBSENTIENT RACES KNOWN TO EXIST BY\n"
    "IMPERIAL SECRET SECURITY.  YOU WILL FIND IT MOUNTED TO THE\n",
    "CONDUIT IN THE MIDDLE OF THE CONTROL CENTER.\n"
    " POINT QUOTA IS 110,000.  GET THE PRIMARY TARGET AND THE QUOTA\n"
    "AND ON YOU GO.\n",
},
{
    "Sublevel 5:\n"
    "The Gauntlet (Trial Zone)\n"
    " THIS TEST AREA IS FOR TRAINING THE KAAL JUMP TROOPS. THE PRIMARY\n"
    "GOAL IS A SINGLE SOYLENT BROWN NARCOTIC,  BUT TO ACQUIRE IT YOU\n"
    "MUST GAIN INFORMATION ON KAAL TROOP TRAINING TECHNIQUES BY\n",
    "RUNNING THEIR GAUNTLET (THE NARCOTIC IS A STANDARD PART OF THE\n"
    "KAAL TROOP REWARD SYSTEM.)\n"
    " NO MAN-TROOPS ARE ON THIS LEVEL SINCE THOSE TROOPS WERE\n",
    "SCRAMBLED DURING FULL ALERT DUE TO THE HUNT.  EXPECT RESISTANCE\n"
    "FROM SENTINALS.  QUOTA IS SET AT 50,000 POINTS.  GET THE PRIMARY\n"
    "AND QUOTA AND WE'LL TRANSLATE YOU TO THE FINAL AREA FOR THE\n"
    "GRAND TEST OF YOUR HUNTER'S SKILLS!\n",
},
{
    "Sublevel 6:\n"
    "Command Bunker\n"
    " THIS IS IT.  THIS ONE COULD WIN YOU THE JACKPOT...\n\n"
    " IT IS WELL KNOWN THAT THE KAAL CHANCELLOR'S POWER IS IN THE\n"
    "IMPERIAL SYGIL OF HIS POSITION.  TO POSSESS IT IS TO BE\n"
    "CHANCELLOR.  WHETHER BY FOOLISHNESS OR ARROGANCE THE CHANCELLOR\n",
    "HAS TAKEN TO REMOVING THE SYGIL FROM HIS PERSON AND LEAVING IT\n"
    "UNGUARDED.  THE SYGIL ITSELF IS ON A PEDESTAL BEYOND THE COMMAND\n"
    "BUNKER...  BEFORE YOU CAN ENTER THIS AREA YOU MUST FIND THE\n"
    "SECURITY KEY WHICH WILL ALLOW YOU TO ENTER HIS QUARTERS.\n",
    " RETRIEVE THE SYGIL AND THE A.V.C. WILL GRANT YOU FAME, FORTUNE\n"
    "AND YOUR FREEDOM...\n"
    "...FAIL, AND YOU'LL FIND YOUR BURNING ENTRAILS FALLING FROM HIGH\n"
    "ORBIT.\n",

},
/* DEMO DATA */

#else

    "LEVEL 2: PRIMARY HOLDING\n"
    "THIS SUBLEVEL SERVES AS THE SUPPLIES ENTRY POINT ON THE\n"
    "COLONY AS WELL AS THE INMATE FREEHOLD.\n"
    "THE PRIMARY OBJECT FOR THIS SUBLEVEL IS A DESARIAN SPACE\n"
    "GENERATOR WHICH HELPS POWER THE OUTER SHIELDING FOR THE\n"
    "RING.  SECONDARY TARGETS INCLUDE VIALS OF TRUTH SERUM\n",
    "AND HYPODERMIC NEEDLES USED DURING INMATE INTERROGATION.\n"
    "ONCE YOU'VE ACQUIRED THE PRIMARY GOAL AND YOUR POINT\n"
    "TOTAL MEETS OR EXCEEDS 55000 WE'LL OPEN A TRANSLATION\n"
    "NEXUS TO THE NEXT SUBLEVEL.\n"
    "OH, AND ONE LAST THING...\n",
    "...DON'T TRIGGER THE AIRLOCKS IN THIS AREA UNLESS YOU\n"
    "WANT THE AIR RIPPED VIOLENTY FROM YOUR LUNGS.",

    "LEVEL 3: DETENTION FACILITY A\n"
    "THE RIOT IN THE COLONY HAS REACHED NEW HEIGHTS OF CHAOS.\n"
    "THE WARDEN'S QUARTERS HAVE BEEN BREACHED AND SUSEQUENTLY,\n"
    "HIS HEAD IS HIDDEN IN A JAR ON THIS SUBLEVEL.  THE A.V.C.\n"
    "IS INTERESTED IN REVIVING THE BRAIN SO IT HAS BECOME YOUR\n",
    "NEXT PRIMARY TARGET ITEM.  SECONDARIES INCLUDE LUBRICANTS\n"
    "AND EMERGENCY LANTERNS.\n",
    "IF AFTER ACQUIRING THE PRIMARY ITEM YOUR POINT TOTAL MEETS\n"
    "OR EXCEEDS 60000 POINTS WE'LL OPEN A NEXUS AND TRANSLATE\n"
    "YOU TO THE PURIFICATION FACILITY.\n"
    "HAPPY HEAD HUNTING.",


    // demo end
    "YOU HAVE PROVEN YOURSELF TO BE A FORMIDABLE HUNTER.\n"
    "KNOW THEN THAT YOU HAVE EARNED THE COVETED SECRET PHRASE:\n"
    "BLACK DOVE FRONT\n"
    "UTTER OR INSCRIBE THE SECRET PHRASE WHEN YOU ORDER THE\n"
    "FULL VERSION OF 'IN PURSUIT OF GREED' DIRECTLY FROM\n"
    "SOFTDISK AND YOU'LL GET FREE SHIPPING!! THE FULL GAME OF\n"
    "'IN PURSUIT OF GREED' WILL SHIP IN EARLY JANUARY, SO GET\n"
    "YOUR COPY ON CD-ROM NOW, FOR JUST 39.95 US DOLLARS.\n",
    "CALL 1-800-831-2694 OR 1-318-221-8718 TO ORDER BY PHONE\n"
    "WITH A CREDIT CARD OR MAIL CHECK OR MONEY ORDER FOR 39.95\n"
    "TO: SOFTDISK PUBLISHING GREED DEMO OFFER #GDC115,\n"
    "P.O. BOX 30008. SHREVEPORT, LA 71130-0008\n",
    "BE SURE TO MENTION THE SECRET PHRASE\n"
    "ALSO, BE SURE TO VISIT OUR INTERNET SITE AT\n"
    "HTTP://WWW.SOFTDISK.COM\n"
    "YOU CAN BE AS TWISTED AS YOU'VE ALWAYS WANTED TO BE!!\n"
    "WHAT ARE YOU WAITING FOR? CALL NOW!!\n",


// "YOU MUST BE ASKING YOURSELF, 'WHERE THE HELL IS THE\n"
// "PURIFICATION FACILITY?' THE ANSWER IS SIMPLE.  IT'S IN\n"
// "THE FULL VERSION WITH THE BYZANTIUM RING AND SCADS OF\n"
// "GUARDS AND TROOPERS GUARDING IT.  ORDER NOW AND THIS\n"
// "IS WHAT YOU CAN EXPECT... ANCIENT ARTIFACTS, EXPERIMENTAL TECH,\n"
// "RARE LIFE FORMS: CANNON FODDER!\n",
// "APPROACH NEW LEVELS OF INSANITY AS EVERYTHING YOU SHOOT\n"
// "DETONATES AND SHATTERS - CHAIRS EXPLODE, STATUES CRUMBLE -\n"
// "ON THREE COMPLETE WORLDS SPANNING OVER 30 LEVELS OF ACTION.\n"
// "USE NEW ITEMS LIKE THE HOLOSUIT, THE WARP JAMMER, AND SOUL \n"
// "STEALER TO HUMILIATE AND CRUSH YOUR ENEMIES IN NETWORK AND\n"
// "MODEM PLAY.  KILL A PAGAN GOD AND WATCH THE WORSHIPPERS \n",
// "SCATTER.  STEAL THE POWER CORE FROM A MOON SIZED PRISON\n"
// "COLONY AND JETTISON THE BODIES THAT REMAIN.\n"
// "CUT THE IMPERIAL SIGNET RING FROM THE CHANCELLOR'S HAND AND\n"
// "CLAIM THE SEAT OF POWER FOR YOURSELF.\n"
// "YOU CAN BE THE TWISTED BASTARD YOU'VE ALWAYS WANTED TO BE!!!\n"
// "WHAT ARE YOU WAITING FOR?\n",

/* "LEVEL 2: TRAINING GROUNDS\n"
 "AS WELL AS PREPARING THEMSLEVES PSYCHICALLY, THE ACOLYTES OF\n"
 "THE AKI-VORTELASH ORDER ARE GIVEN TO THE CARE AND TRAINING\n"
 "OF WAR SLUGS AS PART OF THEIR DISCIPLINE.  IT IS FOR THIS\n"
 "REASON THAT THE PRIESTHOOD COMBINED THE SLUG LARVA HOLDS\n"
 "AND THEIR MARTIAL TRAINING GROUNDS.\n\n",
 "THE PRIMARY TARGET FOR THIS LEVEL IS THE BOOK OF CHANTS;\n"
 "SECONDARY OBJECTS INCLUDE WAR SLUG LARVAE AND SLUG FOOD.\n"
 "SINCE THE BOOK WAS CONSTRUCTED BY THE ANCIENT TRIBE-OF-NINE,\n"
 "ITS VALUE IS OBVIOUS.  THE SLUG LARVAE, HOWEVER, ARE OF MINOR\n"
 "BIOLOGICAL INTEREST AND SHOULD BE TAKEN ONLY WHEN POSSIBLE.\n\n",
 "WHEN YOUR POINT TOTAL MEETS OR EXCEEDS 100000, A TRANSLATION\n"
 "NEXUS WILL BE OPENED TO SEND YOU TO THE NEXT SUBLEVEL.  WE'LL\n"
 "TRANSMIT FURTHER INSTRUCTIONS PENDING YOUR ARRIVAL AT THE\n"
 "TEMPLE DUNGEONS.\n\nGOD SPEED.",

 "LEVEL 3: TEMPLE DUNGEONS\n"
 "THIS AREA SERVES MERELY AS A WAYPOINT TO THE NEXT SUBLEVEL.\n"
 "THERE WAS NO EASY WAY TO TANSLATE YOU ALL THE WAY INTO THE\n"
 "SUMMONING CHAMBERS SO YOU'LL HAVE TO MOVE THROUGH THE TEMPLE\n"
 "DUNGEONS TO GET CLOSE ENOUGH FOR IT.  THERE ISN'T ANYTHING\n",
 "OF REAL VALUE HERE, BUT YOU HAVE TO STEAL SOMETHING SO HERE\n"
 "GOES.  THE PRIMARY TARGET FOR THIS AREA WILL BE THE\n"
 "INCANTATION BRAZIER KEPT ATOP A PYRAMID DEEP WITHIN THE\n"
 "DUNGEON.  THE SECONDARY OBJECTS WILL BE CEREMONIAL CANDLES\n"
 "STREWN ABOUT THE LEVEL.\n",
 "AFTER YOU HAVE ACQUIRED THE PRIMARY OBJECT AND YOUR POINT\n"
 "TOTAL MEETS OR EXCEEDS 100000 WE'LL... EH... YOU KNOW THE\n"
 "ROUTINE.\n",

 // demo end
 "YOU MUST BE ASKING YOURSELF, 'WHERE THE HELL ARE THE\n"
 "SUMMONING CHAMBERS?' THE ANSWER IS SIMPLE.  THEY'RE IN\n"
 "THE FULL VERSION ALONG WITH THE PAGAN GOD YRK'TAREL AND\n"
 "THE ENCODE OF THE DEMON SAINT.  ORDER NOW AND THIS IS WHAT\n"
 "YOU CAN EXPECT... ANCIENT ARTIFACTS, EXPERIMENTAL TECH, \n"
 "RARE LIFE FORMS: CANNON FODDER!\n",
 "APPROACH NEW LEVELS OF INSANITY AS EVERYTHING YOU SHOOT\n"
 "DETONATES AND SHATTERS - CHAIRS EXPLODE, STATUES CRUMBLE -\n"
 "ON THREE COMPLETE WORLDS SPANNING OVER 30 LEVELS OF ACTION.\n"
 "USE NEW ITEMS LIKE THE HOLOSUIT, THE WARP JAMMER, AND SOUL \n"
 "STEALER TO HUMILIATE AND CRUSH YOUR ENEMIES IN NETWORK AND\n"
 "MODEM PLAY.  KILL A PAGAN GOD AND WATCH THE WORSHIPPERS \n",
 "SCATTER.  STEAL THE POWER CORE FROM A MOON SIZED PRISON\n"
 "COLONY AND JETTISON THE BODIES THAT REMAIN.\n"
 "CUT THE IMPERIAL SIGNET RING FROM THE CHANCELLOR'S HAND AND\n"
 "CLAIM THE SEAT OF POWER FOR YOURSELF.\n"
 "YOU CAN BE THE TWISTED BASTARD YOU'VE ALWAYS WANTED TO BE!!!\n"
 "WHAT ARE YOU WAITING FOR?\n",
*/
#endif
};

int viewSizes[MAXVIEWSIZE * 2] = { 320, 200, 320, 200, 320, 200, 320, 200, 320, 189,
                                   320, 136, 288, 136, 272, 136, 256, 128, 240, 120 };

int viewLoc[MAXVIEWSIZE * 2]
    = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 12, 16, 12, 24, 12, 32, 16, 40, 20 };

int slumps[S_END - S_START + 1];

char* slumpnames[S_END - S_START + 1] = {
    // player weapons shots
    "pulsebullet", "pulsebullet", "fireball",    "spreadshot",     "pulsebullet", "crossshot",
    "spec7shot",   "pulsebullet", "prongshot",   "pinkball",       "missile",     "greenball",


    "explode",     "explode2",    "pulsebullet", "firewall",  // mine stuff
    "pulsebullet",                                            // hand weapon attack
    "pulsebullet",                                            // soul stealer bullet
    "wallpuff",    "blood",       "greedblood",  "plasmawallpuff", "greenring",   "explode",
    "generator",   "warp",

    "pulsebullet", "fireball",    "probebullet", "missile",        "spreadshot",  // kaal

    "missile",     "pulsebullet",                               // 6 & 7
    "fireball",    "pulsebullet", "pulsebullet", "spreadshot",  // prison

    "bigredball",  "greenball",   "greenarrow",  "pulsebullet",  // 12, 13, 14, 15
};


char* charnames[MAXCHARTYPES]
    = { "cyborg", "lizardman", "mooman", "specimen7", "dominatrix", "bighead" };


int statusbarloc[MAXVIEWSIZE * 2] = {
    0, 0, 220, 149, 4, 149, 4, 149, 4, 138, 4, 149, 4, 149, 4, 149, 4, 149, 4, 149,
};

fixed_t pheights[MAXCHARTYPES] = {
    42 << FRACBITS, 35 << FRACBITS, 50 << FRACBITS, 35 << FRACBITS, 38 << FRACBITS, 32 << FRACBITS,
};

int pmaxshield[MAXCHARTYPES] = { 700, 400, 500, 300, 500, 700 };

int pmaxangst[MAXCHARTYPES] = { 600, 400, 700, 500, 500, 700 };

int pwalkmod[MAXCHARTYPES] = {
    -PLAYERMOVESPEED, PLAYERMOVESPEED, -PLAYERMOVESPEED, PLAYERMOVESPEED, 0, PLAYERMOVESPEED * 1.5,
};

int prunmod[MAXCHARTYPES] = {
    -PLAYERMOVESPEED,    PLAYERMOVESPEED / 2, -PLAYERMOVESPEED, PLAYERMOVESPEED / 2, 0,
    PLAYERMOVESPEED * 2,
};

fixed_t pjumpmod[MAXCHARTYPES] = {
    -FRACUNIT, 0, 0, 0, -FRACUNIT, 0,
};
