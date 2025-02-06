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

#include "joncd.h" 

#include <stdio.h>
#include <stdlib.h>

#include <string.h>


#define CD_DELAY 1500


void Device_Request( void *block ) {
	(void)block;
} 



int Cdrom_Installed( void ) {
	return(1); /* Johnm 12/1/2001 */
}

/* Used for ejecting tray, closing tray...etc */
void Cd_Command( unsigned char comcode ) {
	(void)comcode;
}

/* Used for ejecting tray, closing tray...etc */
void Cd_Lock( unsigned char lock_mode ) {
	(void)lock_mode;
} 

void Cd_Status( void ) {

} 


void Get_Audio_Info( void ) {

}

uint32_t Track_Pos( int tracknum ) {
	(void)tracknum;
     return(1); //Johnm 12/1/2001
} 

void Play_Song( int tracknum ) {
	(void)tracknum;
} 

/* Set cdrom output volume...0-255 */
void Set_Cd_Volume( int volume ) {
	(void)volume;
} /* End of Set_Cd_Volume */



void Stop_Audio( void ) {
} 

int Is_Audio_Playing( void ) {
	return(1); //Johnm 12/1/2001
}


