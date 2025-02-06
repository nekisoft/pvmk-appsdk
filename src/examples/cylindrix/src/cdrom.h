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

#ifndef CDROM_H
#define CDROM_H

#include "types.h"

#define EJECT_TRAY 0
#define RESET 2
#define CLOSE_TRAY 5
#define DATA_TRACK 64
#define LOCK 1
#define UNLOCK 0

typedef struct playinfo {
  uint8_t control PACKED_STRUCT;
  uint8_t adr     PACKED_STRUCT;
  uint8_t track   PACKED_STRUCT;
  uint8_t index   PACKED_STRUCT;
  uint8_t min     PACKED_STRUCT;
  uint8_t sec     PACKED_STRUCT;
  uint8_t frame   PACKED_STRUCT;
  uint8_t zero    PACKED_STRUCT;
  uint8_t amin    PACKED_STRUCT;
  uint8_t asec    PACKED_STRUCT;
  uint8_t aframe  PACKED_STRUCT;
} playinfo_type;

typedef struct volumeinfo {
    uint8_t mode    PACKED_STRUCT;
    uint8_t input0  PACKED_STRUCT;
    uint8_t volume0 PACKED_STRUCT;
    uint8_t input1  PACKED_STRUCT;
    uint8_t volume1 PACKED_STRUCT;
    uint8_t input2  PACKED_STRUCT;
    uint8_t volume2 PACKED_STRUCT;
    uint8_t input3  PACKED_STRUCT;
    uint8_t volume3 PACKED_STRUCT;
} volumeinfo_type;

typedef struct {
  uint16_t drives             PACKED_STRUCT;
  uint8_t  first_drive        PACKED_STRUCT;
  uint16_t current_track      PACKED_STRUCT;
  uint32_t  track_position     PACKED_STRUCT;
  uint8_t  track_type         PACKED_STRUCT;
  uint8_t  low_audio          PACKED_STRUCT;
  uint8_t  high_audio         PACKED_STRUCT;
  uint8_t  disk_length_min    PACKED_STRUCT;
  uint8_t  disk_length_sec    PACKED_STRUCT;
  uint8_t  disk_length_frames PACKED_STRUCT;
  uint32_t  endofdisk          PACKED_STRUCT;
  uint8_t  upc[7]             PACKED_STRUCT;
  uint8_t  diskid[6]          PACKED_STRUCT;
  uint32_t  status             PACKED_STRUCT;
  uint16_t error              PACKED_STRUCT;
} cdrom_data_type;



void Allocate_Dos_Buffer( void );
void Copy_Into_Dos_Buffer( void *block, short length );
void Copy_From_Dos_Buffer( void *block, short length );
void cd_get_volume (struct volumeinfo *vol);
void cd_set_volume (struct volumeinfo *vol);
void cd_get_audio_info (void);
void cd_set_track (short tracknum);
void cd_status (void);
void cd_seek (uint32_t location);
void cd_play_audio (uint32_t begin, uint32_t end);
void cd_stop_audio (void);
void cd_resume_audio (void);
void cd_cmd (uint8_t mode);
short cdrom_installed (void);
short cd_done_play (void);
void cd_lock (uint8_t doormode);
void Set_CD_Volume( uint8_t volume );

#endif

