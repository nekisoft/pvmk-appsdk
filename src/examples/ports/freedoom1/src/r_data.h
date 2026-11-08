// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//
//-----------------------------------------------------------------------------


#ifndef __R_DATA__
#define __R_DATA__

#include "r_defs.h"
#include "r_state.h"

#ifdef __GNUG__
#pragma interface
#endif

// Retrieve column data for span blitting.
byte*
R_GetColumn
( int		tex,
  int		col );
  
//Gets successor column for bilinear interp
byte*
R_GetColumn2
( int		tex,
  int		col );


// I/O, setting up the stuff.
void R_InitData (void);
void R_PrecacheLevel (void);


// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
int R_FlatNumForName (char* name);


// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
int R_TextureNumForName (char *name);
int R_CheckTextureNumForName (char *name);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
//Brightness rows, built by I_SetPalette. Index with a level from R_ShadeForLevel31(), or
//convert one of Doom's colormaps with R_ShadeForColormap() for paths that still use them.
extern uint16_t shadetab[256][256];
extern uint32_t sparseshadetab[256][256];
extern uint16_t *dc_shade;
extern uint16_t *ds_shade;
extern uint32_t *dc_sparseshade;
extern uint32_t *ds_sparseshade;
uint16_t *R_ShadeForColormap(lighttable_t *cm);
uint16_t *R_ShadeForLevel31(int lvl8);
uint32_t *R_SparseForColormap(lighttable_t *cm);
uint32_t *R_SparseForLevel31(int lvl8);
void R_BuildShadeTables(void);
extern uint8_t r_palrgb[768];
