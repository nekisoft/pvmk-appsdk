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
#include <math.h>
#include <malloc.h>

#include <fcntl.h>
#include "keys.h"
#include "util.h"
#include "prim.h" /* Temporary for setvideomode */

#include <sc.h> //pvmk syscalls - emulate keys with gamepad

extern game_configuration_type game_configuration;


/* variable to hold current key being pressed */
int raw_key;

/* Array to store keypresses */
int key_table[KEY_TABLE_SIZE];
/* Array to store keys releases only (for typing and such */
int break_table[KEY_TABLE_SIZE];


boolean down_table[ KEY_TABLE_SIZE ];


/* Functions  */

/* This function takes whatever number you give it, and if it
   is a break code, returns a character */
int Find_Key( int scan_code, char *ch )
    {
     int return_value = 1;

     switch( scan_code )
         {
          case BREAK_A :
              *ch = 'a';
              break;
          case BREAK_B :
              *ch = 'b';
              break;
          case BREAK_C :
              *ch = 'c';
              break;
          case BREAK_D :
              *ch = 'd';
              break;
          case BREAK_E :
              *ch = 'e';
              break;
          case BREAK_F :
              *ch = 'f';
              break;
          case BREAK_G :
              *ch = 'g';
              break;
          case BREAK_H :
              *ch = 'h';
              break;
          case BREAK_I :
              *ch = 'i';
              break;
          case BREAK_J :
              *ch = 'j';
              break;
          case BREAK_K :
              *ch = 'k';
              break;
          case BREAK_L :
              *ch = 'l';
              break;
          case BREAK_M :
              *ch = 'm';
              break;
          case BREAK_N :
              *ch = 'n';
              break;
          case BREAK_O :
              *ch = 'o';
              break;
          case BREAK_P :
              *ch = 'p';
              break;
          case BREAK_Q :
              *ch = 'q';
              break;
          case BREAK_R :
              *ch = 'r';
              break;
          case BREAK_S :
              *ch = 's';
              break;
          case BREAK_T :
              *ch = 't';
              break;
          case BREAK_U :
              *ch = 'u';
              break;
          case BREAK_V :
              *ch = 'v';
              break;
          case BREAK_W :
              *ch = 'w';
              break;
          case BREAK_X :
              *ch = 'x';
              break;
          case BREAK_Y :
              *ch = 'y';
              break;
          case BREAK_Z :
              *ch = 'z';
              break;
          case BREAK_SPACE :
              *ch = ' ';
              break;
          default :
              return_value = 0;
              break;

         } /* End case */

     return( return_value );

    } /* End find_key */

//int g_AllegroLookup[255];

/* This function Sets the keyboard interrupt pointer
   To point to out New_Key_Int function */
void Init_Keys( void ) {



/*
#define INDEX_F1                33
#define INDEX_F2                34
#define INDEX_F3                35
#define INDEX_F4                36
#define INDEX_F5                37
#define INDEX_F6                38
#define INDEX_F7                39
#define INDEX_F8                40
#define INDEX_F9                41
#define INDEX_F10               42
#define INDEX_F11               43
#define INDEX_F12               44
#define INDEX_ENTER             45
#define INDEX_1                 46
#define INDEX_2                 47
#define INDEX_3                 48
#define INDEX_4                 49
#define INDEX_5                 50
#define INDEX_6                 51
#define INDEX_7                 52
#define INDEX_8                 53
#define INDEX_9                 54
#define INDEX_0                 55
#define INDEX_MINUS             56 
#define INDEX_PLUS              57 
#define INDEX_LEFT_BRACKET      58 
#define INDEX_RIGHT_BRACKET     59 
#define INDEX_BACKSLASH         60 
#define INDEX_SEMICOLON         61 
#define INDEX_APOSTROPHE        62 
#define INDEX_LESS_THAN         63 
#define INDEX_GREATER_THAN      64 
#define INDEX_SLASH             65 
#define INDEX_TILDA             66 
#define INDEX_TAB               67 
#define INDEX_LEFT_SHIFT        68
#define INDEX_RIGHT_SHIFT       69
#define INDEX_BACKSPACE         70
*/

	//g_AllegroLookup[KEY_ESC] = INDEX_ESC;
}





int Check_Key_Table( int index ) {
	return(0); //Johnm 12/1/2001

	if( index == INDEX_LEFT )
         {
          if( key_table[ INDEX_ALT ] == 1 )
              return 0;
          else if( key_table[ INDEX_LEFT ] == 1 )
              return 255;
          else
              return 0;
         }

     if( index == INDEX_RIGHT )
         {
          if( key_table[ INDEX_ALT ] == 1 )
              return 0;
          else if( key_table[ INDEX_RIGHT ] == 1 )
              return 255;
          else
              return 0;
         }

     if( index == INDEX_SIDESTEP_LEFT )
         {
          if( key_table[ INDEX_ALT ] == 0 )
              return 0;
          else if( key_table[ INDEX_LEFT ] == 1 )
              return 255;
          else
              return 0;
         }

     if( index == INDEX_SIDESTEP_RIGHT )
         {
          if( key_table[ INDEX_ALT ] == 0 )
              return 0;
          else if( key_table[ INDEX_RIGHT ] == 1 )
              return 255;
          else
              return 0;

         }


     if( key_table[ index ] )
         return 255;
     else
         return 0;

}


// Return the value in raw_key 
int Check_Raw( void ) {
      return raw_key;
}

/* Reinstall the old Keyboard interrupt */
void Kill_Keys( void ) {
} /* End of Kill_Keys */


void Get_Keypress( void ) {
}  // End of Get_Keypress 

//If the user holds down the mode switch key the ship will keep
//Switching between air and ground rapidly, resulting in nothing happening
//This flag emulates the original Cylindrix behavior which was to only switch
//Modes on the keyup
int g_ModeSwitch = 0;


/* OK...now the input table and the key table are different...
   so we must change the input table based on the key table...
   eventually which keys map to what should me loaded from a
   file and configurable by the user.  For now it is hardcoded */
void Get_Key_Table( input_table table ) {
	int i;


//Johnm 12/2/2001 - NEED TO COMPLETE FUNCTIONALITY HERE
 for( i = 0; i < INPUT_TABLE_SIZE; i++ ) {     table[i] = 0; }

 	
	
     
     KeyboardConfig *config; 

     
     config = &game_configuration.keyboard_config;


     for( i = 0; i < INPUT_TABLE_SIZE; i++ )
         table[i] = 0;

     if( key_table[config->up] )
         table[INDEX_UP] = 255;
     if( key_table[config->down] )
         table[INDEX_DOWN] = 255;
     if( key_table[config->left] )
         table[INDEX_LEFT] = 255;
     if( key_table[config->right] )
         table[INDEX_RIGHT] = 255;

     if( key_table[config->missile] )
         table[INDEX_FIRE_MISSILE] = 1;
     if( key_table[config->laser] )
         table[INDEX_FIRE_GUN] = 1;

     if( key_table[config->up_throttle] )
         table[INDEX_UP_THROTTLE] = 1;
     if( key_table[config->down_throttle] )
         table[INDEX_DOWN_THROTTLE] = 1;


     if( key_table[config->strafe] )
         {
          if( key_table[config->right] )
              {
               table[INDEX_RIGHT] = 0;
               table[INDEX_SIDESTEP_RIGHT] = 255;
              }
          if( key_table[config->left] )
              {
               table[INDEX_LEFT] = 0;
               table[INDEX_SIDESTEP_LEFT] = 255;
              }

          if( key_table[config->up] )
              {
               table[INDEX_UP] = 0;
               table[INDEX_SIDESTEP_UP] = 255;
              }
          if( key_table[config->down] )
              {
               table[INDEX_DOWN] = 0;
               table[INDEX_SIDESTEP_DOWN] = 255;
              }

         }

     if( key_table[config->special] )
         {
          table[INDEX_SPECIAL_WEAPON] = TRUE;
         }

     if( key_table[config->wing_1_pylon] )
           table[INDEX_L_COMMAND_PYLON] = TRUE;
     if( key_table[config->wing_1_attack] )
           table[INDEX_L_COMMAND_ATTACK] = TRUE;
     if( key_table[config->wing_1_attack_rb] )
           table[INDEX_L_COMMAND_ATTACK_RADAR] = TRUE;
     if( key_table[config->wing_1_defend_rb] )
           table[INDEX_L_COMMAND_DEFEND_RADAR] = TRUE;
     if( key_table[config->wing_1_group] )
           table[INDEX_L_COMMAND_GROUP] = TRUE;
     if( key_table[config->wing_1_cancel] )
           table[INDEX_L_COMMAND_CANCEL] = TRUE;
     if( key_table[config->wing_2_pylon] )
           table[INDEX_R_COMMAND_PYLON] = TRUE;
     if( key_table[config->wing_2_attack] )
           table[INDEX_R_COMMAND_ATTACK] = TRUE;
     if( key_table[config->wing_2_attack_rb] )
           table[INDEX_R_COMMAND_ATTACK_RADAR] = TRUE;
     if( key_table[config->wing_2_defend_rb] )
           table[INDEX_R_COMMAND_DEFEND_RADAR] = TRUE;
     if( key_table[config->wing_2_group] )
           table[INDEX_R_COMMAND_GROUP] = TRUE;
     if( key_table[config->wing_2_cancel] )
           table[INDEX_R_COMMAND_CANCEL] = TRUE;


     if( break_table[config->mode_switch] )
         {
          table[INDEX_MODE_SWITCH] = 1;
          break_table[config->mode_switch] = 0;
         }

} /* End of Get_Key_Table */


int Check_Break_Table( int index ) {
     if( break_table[index] )
         return 1;
     else
         return 0;
}

void Clear_Break_Table( void ) {
     int i;

     for( i = 0; i < KEY_TABLE_SIZE; i++ )
         break_table[i] = 0;
}


int Check_Input_Table( int index, input_table table ) {
     if( table[ index ] )
         return table[ index ];
     else
         return 0;
}


int First_Break( void ) {
     int i;


     for( i = 0; i < KEY_TABLE_SIZE; i++ )
         {
          if( break_table[i] != 0 )
              {
               return( break_table[i] );
              }
         }

     return(-1);

} /* End of First_Break() */


void Break_Code_To_String( string scan_string, int break_code ) {
	(void)scan_string;
	(void)break_code;
} /* End of Break_Code_To_String */


int Break_Code_To_Index( int break_code ) {
	(void)break_code;
	return(1);
} /* End of Break_To_Index() */



void Index_To_String( string scan_string, int index ) {
     switch( index )
         {
          case INDEX_A :
              sprintf(scan_string, "A" );
              break;
          case INDEX_B :
              sprintf(scan_string, "B" );
              break;
          case INDEX_C :
              sprintf(scan_string, "C" );
              break;
          case INDEX_D :
              sprintf(scan_string, "D" );
              break;
          case INDEX_E :
              sprintf(scan_string, "E" );
              break;
          case INDEX_F :
              sprintf(scan_string, "F" );
              break;
          case INDEX_G :
              sprintf(scan_string, "G" );
              break;
          case INDEX_H :
              sprintf(scan_string, "H" );
              break;
          case INDEX_I :
              sprintf(scan_string, "I" );
              break;
          case INDEX_J :
              sprintf(scan_string, "J" );
              break;
          case INDEX_K :
              sprintf(scan_string, "K" );
              break;
          case INDEX_L :
              sprintf(scan_string, "L" );
              break;
          case INDEX_M :
              sprintf(scan_string, "M" );
              break;
          case INDEX_N :
              sprintf(scan_string, "N" );
              break;
          case INDEX_O :
              sprintf(scan_string, "O" );
              break;
          case INDEX_P :
              sprintf(scan_string, "P" );
              break;
          case INDEX_Q :
              sprintf(scan_string, "Q" );
              break;
          case INDEX_R :
              sprintf(scan_string, "R" );
              break;
          case INDEX_S :
              sprintf(scan_string, "S" );
              break;
          case INDEX_T :
              sprintf(scan_string, "T" );
              break;
          case INDEX_U :
              sprintf(scan_string, "U" );
              break;
          case INDEX_V :
              sprintf(scan_string, "V" );
              break;
          case INDEX_W :
              sprintf(scan_string, "W" );
              break;
          case INDEX_X :
              sprintf(scan_string, "X" );
              break;
          case INDEX_Y :
              sprintf(scan_string, "Y" );
              break;
          case INDEX_Z :
              sprintf(scan_string, "Z" );
              break;
          case INDEX_1 :
              sprintf(scan_string, "1" );
              break;
          case INDEX_2 :
              sprintf(scan_string, "2" );
              break;
          case INDEX_3 :
              sprintf(scan_string, "3" );
              break;
          case INDEX_4 :
              sprintf(scan_string, "4" );
              break;
          case INDEX_5 :
              sprintf(scan_string, "5" );
              break;
          case INDEX_6 :
              sprintf(scan_string, "6" );
              break;
          case INDEX_7 :
              sprintf(scan_string, "7" );
              break;
          case INDEX_8 :
              sprintf(scan_string, "8" );
              break;
          case INDEX_9 :
              sprintf(scan_string, "9" );
              break;
          case INDEX_0 :
              sprintf(scan_string, "0" );
              break;

          case INDEX_UP_ARROW :
              sprintf(scan_string, "UP ARROW" );
              break;
          case INDEX_DOWN_ARROW :
              sprintf(scan_string, "DOWN ARROW" );
              break;
          case INDEX_RIGHT_ARROW :
              sprintf(scan_string, "RIGHT ARROW" );
              break;
          case INDEX_LEFT_ARROW :
              sprintf(scan_string, "LEFT ARROW" );
              break;

          case INDEX_CONTROL :
              sprintf(scan_string, "CONTROL");
              break;
          
          case INDEX_ALT :
              sprintf(scan_string, "ALT");
              break;

          case INDEX_SPACE :
              sprintf(scan_string, "SPACE BAR");
              break;


          case INDEX_MINUS :
              sprintf(scan_string, "MINUS");
              break;

          case INDEX_PLUS :
              sprintf(scan_string, "PLUS");
              break;

          case INDEX_LEFT_BRACKET :
              sprintf(scan_string, "LEFT BRACKET");
              break;

          case INDEX_RIGHT_BRACKET :
              sprintf(scan_string, "RIGHT BRACKET");
              break;

          case INDEX_BACKSLASH :
              sprintf(scan_string, "BACKSLASH");
              break;

          case INDEX_SEMICOLON :
              sprintf(scan_string, "SEMICOLON");
              break;

          case INDEX_APOSTROPHE :
              sprintf(scan_string, "APOSTROPHE");
              break;

          case INDEX_LESS_THAN :
              sprintf(scan_string, "LESS THAN");
              break;

          case INDEX_GREATER_THAN :
              sprintf(scan_string, "GREATER THAN");
              break;

          case INDEX_SLASH :
              sprintf(scan_string, "SLASH");
              break;

          case INDEX_TILDA :
              sprintf(scan_string, "TILDA");
              break;

          case INDEX_TAB :
              sprintf(scan_string, "TAB");
              break;


          case INDEX_F1 :
              sprintf(scan_string, "F1");
              break;
          case INDEX_F2 :
              sprintf(scan_string, "F2");
              break;
          case INDEX_F3 :
              sprintf(scan_string, "F3");
              break;
          case INDEX_F4 :
              sprintf(scan_string, "F4");
              break;
          case INDEX_F5 :
              sprintf(scan_string, "F5");
              break;
          case INDEX_F6 :
              sprintf(scan_string, "F6");
              break;
          case INDEX_F7 :
              sprintf(scan_string, "F7");
              break;
          case INDEX_F8 :
              sprintf(scan_string, "F8");
              break;
          case INDEX_F9 :
              sprintf(scan_string, "F9");
              break;
          case INDEX_F10 :
              sprintf(scan_string, "F10");
              break;
          case INDEX_F11 :
              sprintf(scan_string, "F11");
              break;
          case INDEX_F12 :
              sprintf(scan_string, "F12");
              break;

          /*
          case INDEX_LEFT_SHIFT :
              sprintf(scan_string, "LEFT SHIFT");
              break;
          */
          case INDEX_RIGHT_SHIFT :
              sprintf(scan_string, "RIGHT SHIFT");
              break;

          case INDEX_ENTER :
              sprintf(scan_string, "ENTER");
              break;

          case INDEX_BACKSPACE :
              sprintf(scan_string, "BACKSPACE");
              break;

          default :
              sprintf(scan_string, "NONE" );
              break;

         }

} /* End of Index_To_String */

void Testit( void ) {
} // End of Testit() 

void pvmk_pollkeys(void)
{
	_sc_input_t input = {0};
	while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
	{
		if(input.format != 'A')
			continue;
		
		static const int map[16] = 
		{
			[_SC_BTNIDX_A] = INDEX_ENTER,
			[_SC_BTNIDX_B] = INDEX_ESC,
			
			[_SC_BTNIDX_UP] = INDEX_UP_ARROW,
			[_SC_BTNIDX_DOWN] = INDEX_DOWN_ARROW,
			[_SC_BTNIDX_LEFT] = INDEX_LEFT_ARROW,
			[_SC_BTNIDX_RIGHT] = INDEX_RIGHT_ARROW,
		};
		
		for(int bb = 0; bb < 16; bb++)
		{
			if(map[bb] != 0)
				key_table[map[bb]] = (input.buttons & (1u << bb)) ? 1 : 0;
		}		
	}
}

boolean Jon_Kbhit( void )
    {
	    pvmk_pollkeys();
     int i;

     for( i = 0; i < KEY_TABLE_SIZE; i++ )
          if( !key_table[i] )
               down_table[i] = FALSE;


     for( i = 0; i < KEY_TABLE_SIZE; i++ )
         if( key_table[i] && !down_table[i] )
             return(TRUE);

     
     return FALSE;
    } /* End of Jon_Kbhit() */      
    

int Jon_Getkey( void ) {
	pvmk_pollkeys();
     for( int i = 0; i < KEY_TABLE_SIZE; i++ )
         if( key_table[i] && !down_table[i] )
             {
              down_table[i] = TRUE;
              return( i );
             }

     return(-1);
} /* End of Jon_Getkey() */ 
