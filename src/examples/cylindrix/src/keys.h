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

/*
* keys.h: routines for installing a keyboard interrupt
* and storing the up, down, left, and right keyboard
* presses in an array
*
* Version 1.0
*/



#ifndef KEYS_H

#define KEYS_H

/* #include <dpmi.h> */
#include "types.h"

#define EXTENDED                224  /* Value of extended MF-II key */

#define KEYBOARD_INT            0x09
#define KEY_BUFFER              0x60
#define KEY_CONTROL             0x61
#define INT_CONTROL             0x20


#define KEY_TABLE_SIZE 100  /* Arbitrary number for now */


/* Make (key down) codes for the keys */

#define MAKE_RIGHT              77
#define MAKE_LEFT               75
#define MAKE_UP                 72
#define MAKE_DOWN               80

#define MAKE_SPACE              57
#define MAKE_CONTROL            29
#define MAKE_ALT                56
/* #define MAKE_LEFT_SHIFT         42 */
#define MAKE_LEFT_SHIFT         54  
#define MAKE_RIGHT_SHIFT        54
#define MAKE_ENTER              28
#define MAKE_BACKSPACE          14

#define MAKE_A                  30
#define MAKE_B                  48
#define MAKE_C                  46
#define MAKE_D                  32
#define MAKE_E                  18
#define MAKE_F                  33
#define MAKE_G                  34
#define MAKE_H                  35
#define MAKE_I                  23
#define MAKE_J                  36
#define MAKE_K                  37
#define MAKE_L                  38
#define MAKE_M                  50
#define MAKE_N                  49
#define MAKE_O                  24
#define MAKE_P                  25
#define MAKE_Q                  16
#define MAKE_R                  19
#define MAKE_S                  31
#define MAKE_T                  20
#define MAKE_U                  22
#define MAKE_V                  47
#define MAKE_W                  17
#define MAKE_X                  45
#define MAKE_Y                  21
#define MAKE_Z                  44

#define MAKE_1                  2 
#define MAKE_2                  3
#define MAKE_3                  4
#define MAKE_4                  5
#define MAKE_5                  6
#define MAKE_6                  7
#define MAKE_7                  8
#define MAKE_8                  9
#define MAKE_9                  10
#define MAKE_0                  11
#define MAKE_MINUS              12
#define MAKE_PLUS               13
#define MAKE_LEFT_BRACKET       26
#define MAKE_RIGHT_BRACKET      27
#define MAKE_BACKSLASH          43
#define MAKE_SEMICOLON          39
#define MAKE_APOSTROPHE         40
#define MAKE_LESS_THAN          51
#define MAKE_GREATER_THAN       52
#define MAKE_SLASH              53
#define MAKE_TILDA              41
#define MAKE_TAB                15




#define MAKE_F1                 59
#define MAKE_F2                 60
#define MAKE_F3                 61
#define MAKE_F4                 62
#define MAKE_F5                 63
#define MAKE_F6                 64
#define MAKE_F7                 65
#define MAKE_F8                 66
#define MAKE_F9                 67
#define MAKE_F10                68
#define MAKE_F11                87
#define MAKE_F12                88
#define MAKE_ESC                1


/* For the break (key up) codes, we add 128 to the make code */
#define BREAK_RIGHT             205
#define BREAK_LEFT              203
#define BREAK_UP                200
#define BREAK_DOWN              208

#define BREAK_SPACE             185
#define BREAK_CONTROL           157
#define BREAK_ALT               184
/* #define BREAK_LEFT_SHIFT        170 */
#define BREAK_LEFT_SHIFT        182 
#define BREAK_RIGHT_SHIFT       182
#define BREAK_ENTER             156
#define BREAK_BACKSPACE         142

#define BREAK_A                 158
#define BREAK_B                 176
#define BREAK_C                 174
#define BREAK_D                 160
#define BREAK_E                 146
#define BREAK_F                 161
#define BREAK_G                 162
#define BREAK_H                 163
#define BREAK_I                 151
#define BREAK_J                 164
#define BREAK_K                 165
#define BREAK_L                 166
#define BREAK_M                 178
#define BREAK_N                 177
#define BREAK_O                 152
#define BREAK_P                 153
#define BREAK_Q                 144
#define BREAK_R                 147
#define BREAK_S                 159
#define BREAK_T                 148
#define BREAK_U                 150
#define BREAK_V                 175
#define BREAK_W                 145
#define BREAK_X                 173
#define BREAK_Y                 149
#define BREAK_Z                 172


#define BREAK_1                 130  
#define BREAK_2                 131
#define BREAK_3                 132 
#define BREAK_4                 133 
#define BREAK_5                 134 
#define BREAK_6                 135 
#define BREAK_7                 136 
#define BREAK_8                 137 
#define BREAK_9                 138 
#define BREAK_0                 139 
#define BREAK_MINUS             140 
#define BREAK_PLUS              141 
#define BREAK_LEFT_BRACKET      154 
#define BREAK_RIGHT_BRACKET     155 
#define BREAK_BACKSLASH         171 
#define BREAK_SEMICOLON         167 
#define BREAK_APOSTROPHE        168 
#define BREAK_LESS_THAN         179 
#define BREAK_GREATER_THAN      180 
#define BREAK_SLASH             181 
#define BREAK_TILDA             169 
#define BREAK_TAB               143 


#define BREAK_F1                187
#define BREAK_F2                188
#define BREAK_F3                189
#define BREAK_F4                190
#define BREAK_F5                191
#define BREAK_F6                192
#define BREAK_F7                193
#define BREAK_F8                194
#define BREAK_F9                195
#define BREAK_F10               196
#define BREAK_F11               215
#define BREAK_F12               216
#define BREAK_ESC               129



/* Indexes into key table */
#define INDEX_A                 0
#define INDEX_B                 1
#define INDEX_C                 2
#define INDEX_D                 3
#define INDEX_E                 4
#define INDEX_F                 5
#define INDEX_G                 6
#define INDEX_H                 7
#define INDEX_I                 8
#define INDEX_J                 9
#define INDEX_K                 10
#define INDEX_L                 11
#define INDEX_M                 12
#define INDEX_N                 13
#define INDEX_O                 14
#define INDEX_P                 15
#define INDEX_Q                 16
#define INDEX_R                 17
#define INDEX_S                 18
#define INDEX_T                 19
#define INDEX_U                 20
#define INDEX_V                 21
#define INDEX_W                 22
#define INDEX_X                 23
#define INDEX_Y                 24
#define INDEX_Z                 25
#define INDEX_UP_ARROW          26
#define INDEX_DOWN_ARROW        27
#define INDEX_LEFT_ARROW        28
#define INDEX_RIGHT_ARROW       29
#define INDEX_SPACE             30
#define INDEX_CONTROL           31
#define INDEX_ALT               32
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
#define INDEX_ESC               71


/* This is the function that is called when we install our own keyboard interrupt */
/*
Johnm 12/1/2001 - Not going to be using a keyboard interrupt anymore
void New_Key_Int( _go32_dpmi_registers *r );          
*/

/* Initialize the keyboard interrupt */
void Init_Keys( void );

int Check_Key_Table( int index );
int Check_Raw( void );    /*  Check what key is pressed right now   */
void Kill_Keys( void );   /* Kill the keyboard interrupt */        
void Get_Keypress( void );
void Get_Key_Table( input_table table );
int Check_Break_Table( int index );
void Clear_Break_Table( void );
int Check_Input_Table( int index, input_table table );

/* Returns the first break code in the break table */
int First_Break( void );
void Break_Code_To_String( string scan_string, int break_code );
void Index_To_String( string scan_string, int index );
int Break_Code_To_Index( int break_code );
void Testit( void );
boolean Jon_Kbhit( void );
int Jon_Getkey( void );

#endif
