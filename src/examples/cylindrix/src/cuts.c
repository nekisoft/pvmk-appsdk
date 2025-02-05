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

#include "cuts.h"
#include "jonsb.h"
#include "fli.h"
#include "util.h"


extern char g_DataPath[255];

extern game_configuration_type game_configuration;


char cdrom_drive_letter = 'd';

string load_string;


void Play_Intro_Animation( void )
    {
     sb_sample *sample_one, *sample_two, *sample_three;
     sb_mod_file *my_mod;


     my_mod = sb_load_mod_file( "anything.mod" );


     sample_one   = sb_load_sample("wav_data/computer/intro1.wav",_SB_WAV);
     sample_two   = sb_load_sample("wav_data/computer/intro2.wav",_SB_WAV);
     sample_three = sb_load_sample("wav_data/computer/intro3.wav",_SB_WAV);

     if( sample_one == NULL || sample_two == NULL || sample_three == NULL )
         {
          fprintf(stderr, "Bad wav in intro \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         {
          sb_mod_play(my_mod);
          Play_Voice( sample_one );
         }

     sprintf(load_string, "%c:/fli/house.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sprintf(load_string, "%c:/fli/logo.fli", cdrom_drive_letter );

     if( game_configuration.sound_on )
         Play_Voice( sample_two );

     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     if( game_configuration.sound_on )
         {
          Play_Voice( sample_three );
          while( !Is_Voice_Done() );
         }

     if( game_configuration.sound_on )
         sb_mod_pause();

     sb_free_sample( sample_one );
     sb_free_sample( sample_two );
     sb_free_sample( sample_three );
     sb_free_mod_file( my_mod );

    } /* End of Play_Intro_Animation() */

void Play_Cut_One( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr1.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/one.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_One() */

void Play_Cut_Two( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr2.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/two.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );


    } /* End of Play_Cut_Two() */

void Play_Cut_Three( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr3.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/three.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_Three() */

void Play_Cut_Four( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr4.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/four.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_Four() */

void Play_Cut_Five( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr5.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/five.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_Five() */

void Play_Cut_Six( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr6.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/six.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_Six() */

void Play_Cut_Seven( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr7.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/seven.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     if( game_configuration.sound_on )
         sb_free_sample( sample_one );

    } /* End of Play_Cut_Seven() */

void Play_Cut_Eight( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr8.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/eight.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);


     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_Eight() */

void Play_Cut_Nine( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr9.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/nine.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );

    } /* End of Play_Cut_Nine() */

void Play_Cut_Ten( void )
    {
     sb_sample *sample_one;

     sample_one   = sb_load_sample("wav_data/computer/narr10.wav",_SB_WAV);

     if( sample_one == NULL )
         {
          fprintf(stderr, "Bad wav in cut one \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         Play_Voice( sample_one );

     sprintf(load_string, "%c:/fli/ten.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );

     sb_free_sample( sample_one );


    } /* End of Play_Cut_Ten() */


void Play_Ending_Animation( void )
    {
     sb_sample *sample_one, *sample_two;
     sb_mod_file *my_mod;


     my_mod = sb_load_mod_file( "anything.mod" );

     sample_one   = sb_load_sample("wav_data/computer/final1.wav",_SB_WAV);
     sample_two   = sb_load_sample("wav_data/computer/final2.wav",_SB_WAV);

     if( sample_one == NULL || sample_two == NULL )
         {
          fprintf(stderr, "Bad wav in intro \n");
          exit_gracefully();
         }

     if( game_configuration.sound_on )
         {
          sb_mod_play(my_mod);
          Play_Voice( sample_one );
         }

     sprintf(load_string, "%c:/fli/final.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         {
          while( !Is_Voice_Done() );
          Play_Voice( sample_two );
         }
     sprintf(load_string, "%c:/fli/fall.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         while( !Is_Voice_Done() );


     sprintf(load_string, "%c:/fli/fade.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);
     
     sprintf(load_string, "%c:/fli/cylindrx.fli", cdrom_drive_letter );
     Play_Fli_Buffered(load_string);

     if( game_configuration.sound_on )
         sb_mod_pause();


     sb_free_mod_file( my_mod );
     sb_free_sample( sample_one );
     sb_free_sample( sample_two );

    } /* End of Play_Ending_Animation() */


int Cylindrix_Disk_Present( void ) {
	return(1); //Johnm 12/2/2001
/*
     FILE *fp;
     string temp_string;


     sprintf(temp_string, "%c:/fli/house.fli", cdrom_drive_letter);

     if( (fp = fopen( temp_string, "rb" )) == NULL )
         {
          return(0);
         }
     else
         {
          fclose(fp);
          return(1);
         }
*/
} /* End of Cylindrix_Disk_Present() */
