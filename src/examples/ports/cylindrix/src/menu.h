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

#include <stdio.h>
#include "types.h"
#include "jonsb.h"
#include "pcx.h"

#ifndef MENU_H
#define MENU_H

typedef void (*Method)(void);
typedef void (*Method2)( int );

typedef struct {
    Method up_arrow;            /* function called when user hits up_arrow */
    Method down_arrow;          /* called when user hits down_arrow */
    Method right_arrow;         /* called when user hits right_arrow */
    Method left_arrow;          /* called when user hits left_arrow */
    Method return_key;          /* called when user hits return_key */
    Method escape_key;          /* called when user htis the escape key */
    Method draw_menu;           /* called when the menu needs to be drawn */
} MenuScreen;

typedef struct {
    string item[50];            /* holds all the menu selections */
    int32_t current_selection;     /* the users currently selected selection */
    int32_t num_items;             /* the number of selections */
} GeneralMenuData;

typedef struct {
    string name;
    string race;
    string description_1;
    string description_2;
    string description_3;
    string description_4;
    enum VehicleType preferred_vehicle;
    sb_sample *sample;
} CharacterInfo;

typedef struct {
    int32_t wingman_index;         /* identifies the player whose ai we are selecting (unused) */
    string menu_title;          /* title of menu */
    CharacterInfo info[39];     /* each characters info (name, race, description ... ) */
    int32_t current_row;
    int32_t current_col;
    int32_t num_items;               /* the number of characters able to be selected must be a multiple of 3 */
    boolean palette_active;       /* true if pallete is active */
    palette_type menu_palette;
} WingmanMenuData;

typedef struct {
    unsigned char shields;
    unsigned char firepower;
    unsigned char air_speed;
    unsigned char surface_speed;
    string special_weapon;
} VehicleInfo;

typedef struct {
    int32_t current_selection;
    string menu_title;
    int32_t num_items;
    VehicleInfo info[8];
    PointFace world_obj;
    Orientation orient;
    string wingman_name;
    string preferred_vehicle_name;
} VehicleMenuData;

void menu_event_loop(void);

int during_game_menu_event_loop(void);

void custom_game_stat_menu_event_loop( int victory );

void training_game_stat_menu_event_loop( int victory );

void null_modem_game_stat_menu_event_loop( int victory );

void tournament_game_stat_menu_event_loop( int victory );

void ipx_game_stat_menu_event_loop( int victory );

void init_all_main_menus(void);

void make_menu_current( MenuScreen *menu );

void character_picture_blit( int32_t anchor_x, int32_t anchor_y, pcx_picture *pcx );

int32_t ai_to_selection( int32_t ai );

#endif
