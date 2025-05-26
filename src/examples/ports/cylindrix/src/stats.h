/*
    Copyright (C) 2001 Hotwarez LLC, Goldtree Enterprises
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; 
    version 2 of the License.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef STATS_H

#define STATS_H

#include <stdint.h>

typedef struct
    {
     char           name[80];       /* This characters name */
     uint16_t kills;          /* How many enemies you killed */
     uint16_t times_killed;   /* How many times character was killed */
     uint16_t shots_fired;    /* How many projectiles you've fired */
     uint16_t enemy_hits;     /* How many times you hit an enemy with a projectile */
     uint16_t friendly_hits;  /* How many times you hit a friend with a projectile */
     uint16_t misses;         /* How many shots you fired that did nothing */
     uint16_t times_hit;      /* How many times you were hit with projectiles */
     int            victory;        /* Did this guy win or lose? 1 or 0 */
     float          hit_percentage; /* Hit to miss ratio */
     uint16_t pylons_grabbed; /* How many pylons you grabbed */
     float          average_d;      /* Your average d throughout the game */

     int            user_vehicle;   /* Current vehicle controlled by user...for internal use */
     uint32_t  total_frames;   /* Number of frames a game took...for internal use */
     uint32_t  total_d;        /* Added d from every frame...used to compute average d */
    } game_stats_type;


typedef struct
    {
     char          name[80];        /* This characters name */
     uint32_t number_of_games; /* How many games you have played (uint32_t...hehe) */
     uint32_t victories;       /* Number of victories */
     uint32_t defeats;         /* Number of defeats */
     float         win_percentage;  /* Percent of the time you won */
     uint32_t kills;           /* How many cats has this cat killed */
     uint32_t times_killed;    /* How many times character was killed */
     uint32_t shots_fired;     /* How many projectiles you've fired */
     uint32_t enemy_hits;      /* How many times you hit an enemy with a projectile */
     uint32_t friendly_hits;   /* How many times you hit an friend with a projectile */
     uint32_t misses;          /* How many shots you fired that did nothing */
     uint32_t times_hit;       /* How many times you were hit with projectiles */
     float         hit_percentage;  /* Hit to miss ratio */
     uint32_t pylons_grabbed;  /* How many pylons you grabbed */
    } overall_stats_type;


/* Save the stats for this game into a text file for the user */
int Save_Game_Stats( game_stats_type *game_stats, char *filename );

/* Save this players overall stats for into a text file */
int Save_Overall_Stats_Text( overall_stats_type *overall_stats, char *filename );

/* Save this players overall stats for into a binary file (used by game) */
int Save_Overall_Stats_Binary( overall_stats_type *overall_stats, char *filename );

/* Load this players overall stats from a binary file */
int Load_Overall_Stats_Binary( overall_stats_type *overall_stats, char *filename );

/* Update this characters overall stats with this game's stats */
void Add_Game_Stats_To_Overall_Stats( game_stats_type *game_stats, overall_stats_type *overall_stats );

void Clear_Game_Stats( game_stats_type *game_stats );
void Clear_Overall_Stats( overall_stats_type *overall_stats );

void Figure_Game_Stats( game_stats_type *game_stats );

#endif
