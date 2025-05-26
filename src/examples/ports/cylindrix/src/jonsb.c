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

#include <stdlib.h>

#include "types.h"

#include "sb_stub.h"

#include "jonsb.h"
#include "util.h"
#include "ai_util.h"
#include "keys.h"



/* Globals */


extern game_configuration_type game_configuration; /* All these from omega.c */
extern sb_sample *sample[MAX_WORLD_SAMPLES];
extern sb_sample *computer_sample[MAX_COMPUTER_SAMPLES];  

static sb_sample new_sample[MAX_JON_SAMPLES];
static sb_sample voice_sample[MAX_VOICE_SAMPLES];
static short sample_index = 0;
static short voice_index = 0;

short num_playing = 0; /* Number of samples currently playing */

/* End of globals */ 

typedef struct {
     short num_samples;                 /* Number of samples qeued */
     sound_index_type sound_index[10];  /* Type of sound events */
     Float_Point point[10];             /* Positions each sound occurred */
} q_sample_type; 


q_sample_type q_samples;




/* struct to store sample info.                                               
typedef struct {
  BYTE *data;
  int length;
  int stereo;
  int bits;
  int left_volume;
  int right_volume;
  void (*callback)();
} sb_sample;
*/


/* This gets called when a sample is done playing */
void My_Callback( void ) {
     num_playing--;

     if( num_playing < 0 )
         {
          fprintf(stdout, "Error in jonsb my callback()\n ");
          Get_Keypress();
          exit_gracefully();
         }
} /* End of My_Callback() */

/* Compute the volume based on the distance between point and orientation */
void Find_Volume( int *left_volume, int *right_volume, Float_Point point, Orientation *o ) {
     /* short horizontal_angle; */ /* Angle the sound occurred from the listener */
     float distance; /* Distance that the sound occurred from the listener */
     short horizontal_angle;
     short temp_angle;


     horizontal_angle = Horizontal_Point_Angle_3D( point, o ); 
     distance = Point_Distance_3D( point, o->position );  

     if( distance > 25 ) /* 60 is all the way across the cylinder */
         {
          *left_volume  = 0;
          *right_volume = 0;
         }
     else /* Distance <= 20 */
         {
          *left_volume  = 25 - distance;
          *right_volume = 25 - distance;
         } /* End if */


    if( horizontal_angle > 5 && horizontal_angle < 175 )
        {
         if( horizontal_angle > 90 )
             temp_angle = horizontal_angle - 90;
         else
             temp_angle = horizontal_angle;

         *right_volume  = *right_volume + (*left_volume * ((float)temp_angle / 90) );
         *left_volume = *left_volume - (*right_volume * ((float)temp_angle / 90) );
         if( *left_volume < 0 )
             *left_volume = 0;
        }
    
    if( horizontal_angle > 185 && horizontal_angle < 355 )
        {
         temp_angle = 360 - horizontal_angle;

         if( temp_angle > 90 )
             temp_angle = temp_angle - 90;
         //else
         //    temp_angle = temp_angle;

         *left_volume  = *left_volume + (*left_volume * ((float)temp_angle / 90) );
         *right_volume = *right_volume - (*right_volume * ((float)temp_angle / 90) );
         
         if( *right_volume < 0 )
             *right_volume = 0;
        }

} /* End of Find_Volume */

/* Figure out volume we should play sample at based on variables in game config */
int Voice_Volume( int volume ) {
     float ratio;
     int return_vol;

     ratio = (float)game_configuration.voice_vol / 255.0;
     return_vol = (float)volume * ratio;
     return(return_vol);

} /* End of Game_Volume */

/* Figure out volume we should play sample at based on variables in game config */
int Fx_Volume( int volume ) {
     float ratio;
     int return_vol;


     ratio = (float)game_configuration.sound_vol / 255.0;
     return_vol = (float)volume * ratio;
     return(return_vol);

} /* End of Fx_Volume */





/* Initialize all the data pointers to NULL so we can use free() freely 
   and init sample index to 0 so we can start at the start */
void Init_Jon_Samples( void ) {
     int i;

     for( i = 0; i < MAX_JON_SAMPLES; i++ )
         {
          new_sample[i].data = NULL;
         }
     
     for( i = 0; i < MAX_VOICE_SAMPLES; i++ )
         {
          voice_sample[i].data = NULL;
         }
     
     sample_index = 0;
     voice_index = 0;
     q_samples.num_samples = 0;

} /* End Init_Jon_Samples */



/* Adjust one samples volume according to the distance between orientation 
   and point */
void Jon_Mix_Sample( sb_sample *old_sample, Float_Point point, Orientation *o ) {

	 if( !old_sample )
		 return;

     new_sample[sample_index].length   = old_sample->length;
     new_sample[sample_index].stereo   = old_sample->stereo;
     new_sample[sample_index].bits     = old_sample->bits;
     new_sample[sample_index].callback = My_Callback;     /* old_sample->callback; */
     
     Find_Volume( &new_sample[sample_index].left_volume, &new_sample[sample_index].right_volume, point, o ); 
     
     new_sample[sample_index].left_volume  = Fx_Volume( new_sample[sample_index].left_volume );
     new_sample[sample_index].right_volume = Fx_Volume( new_sample[sample_index].right_volume );

     if( new_sample[sample_index].left_volume > 5 || new_sample[sample_index].right_volume > 5 )
         {
          /*
          free( new_sample[sample_index].data );
          if( (new_sample[sample_index].data = (BYTE *)malloc(new_sample[sample_index].length) ) == NULL ) 
              {
               printf( "Jon_Mix_Sample() : malloc failed\n" );
               exit_gracefully();
              }
          

          for( i = 0; i < new_sample[sample_index].length; i++ )
              { 
               new_sample[sample_index].data[i] = old_sample->data[i]; 
              }

          */
          new_sample[sample_index].data = old_sample->data; 

          if( num_playing < 14 )
              {

			//Letting sound library handle this
			  //               num_playing++;

               sb_mix_sample( &new_sample[sample_index] );

               if( ++sample_index >= MAX_JON_SAMPLES )
                   sample_index = 0;
              }

         } /* End if volume */

} /* End of Jon_Mix_Sample */



/* Qeueueueu a sample for later playing (later in the frame hopefully)
   and store the position that the sound event occurred */
void Q_Jon_Sample( sound_index_type sound_index, Float_Point point ) {                               
     if( q_samples.num_samples < 6 )
         {
          q_samples.sound_index[q_samples.num_samples] = sound_index;
          q_samples.point[q_samples.num_samples][X]    = point[X];
          q_samples.point[q_samples.num_samples][Y]    = point[Y];
          q_samples.point[q_samples.num_samples][Z]    = point[Z];
          q_samples.num_samples                        += 1;
         }

} /* End of Q_Jon_Sample */


void Play_Q_Samples( Orientation *o ) {
     int i;

     for( i = 0; i < q_samples.num_samples; i++ )
         {
          Jon_Mix_Sample( sample[ q_samples.sound_index[i] ], q_samples.point[i], o );
         } 
     
     q_samples.num_samples = 0;    
     
}  /* End of Play_Q_Samples */



/* Adjust one samples volume according to the distance between orientation 
   and point */
void Mix_Voice_Sample( sb_sample *old_sample ) {

	 if( !old_sample )
		 return;

     voice_sample[voice_index].length        = old_sample->length;
     voice_sample[voice_index].stereo        = old_sample->stereo;
     voice_sample[voice_index].bits          = old_sample->bits;
     voice_sample[voice_index].callback      = old_sample->callback;
     voice_sample[voice_index].left_volume   = old_sample->left_volume;
     voice_sample[voice_index].right_volume  = old_sample->right_volume;      


     voice_sample[voice_index].left_volume  = Voice_Volume( voice_sample[voice_index].left_volume );
     voice_sample[voice_index].right_volume = Voice_Volume( voice_sample[voice_index].right_volume );

     /*
     free( voice_sample[voice_index].data );
     if( (voice_sample[voice_index].data = (BYTE *)malloc(voice_sample[voice_index].length) ) == NULL ) 
         {
          printf( "Mix_Voice_Sample() : malloc failed\n" );
          exit_gracefully();
         }
     
     for( i = 0; i < voice_sample[voice_index].length; i++ )
         { 
          voice_sample[voice_index].data[i] = old_sample->data[i]; 
         }
     */

     voice_sample[voice_index].data = old_sample->data; 

     sb_queue_sample( &voice_sample[voice_index] );
    
     if( ++voice_index >= MAX_VOICE_SAMPLES )
         voice_index = 0;

} /* End of Mix_Voice_Sample */


boolean voice_done = FALSE;

/* Put this in the callback for play_voice to let the caller know voice is done */
void Set_Voice_Done( void ) {
     voice_done = TRUE;
} /* End of Set_Voice_Done() */

boolean Is_Voice_Done( void ) {
	Hack_Update_Queue(); //in sb_stub.c...used to simulate a callback
     return( voice_done );
} /* End of Is_Voice_Done() */

void Play_Voice( sb_sample *old_sample ) {

	if( !old_sample )
		return;

     voice_sample[voice_index].length        = old_sample->length;
     voice_sample[voice_index].stereo        = old_sample->stereo;
     voice_sample[voice_index].bits          = old_sample->bits;
/*      voice_sample[voice_index].callback      = old_sample->callback; */
     voice_sample[voice_index].left_volume   = old_sample->left_volume;
     voice_sample[voice_index].right_volume  = old_sample->right_volume;      
     

     voice_sample[voice_index].callback     = Set_Voice_Done;
     voice_sample[voice_index].left_volume  = Voice_Volume( voice_sample[voice_index].left_volume );
     voice_sample[voice_index].right_volume = Voice_Volume( voice_sample[voice_index].right_volume );


     voice_sample[voice_index].data = old_sample->data; 

     voice_done = FALSE;
     
     sb_queue_sample( &voice_sample[voice_index] );
    
     if( ++voice_index >= MAX_VOICE_SAMPLES )
         voice_index = 0;

} /* End of Play_Voice */


/* Adjust one samples volume according to the distance between orientation
   and point */
void Play_Menu_Sound( sb_sample *old_sample ) {

	if( !old_sample )
		return;
	
	if( old_sample->data )
		New_Play_Menu_Sound(old_sample->data);

	return;

     new_sample[sample_index].length   = old_sample->length;
     new_sample[sample_index].stereo   = old_sample->stereo;
     new_sample[sample_index].bits     = old_sample->bits;
     new_sample[sample_index].callback = old_sample->callback; 
     new_sample[sample_index].left_volume  = old_sample->left_volume;
     new_sample[sample_index].right_volume = old_sample->right_volume;


     new_sample[sample_index].left_volume  = Fx_Volume( new_sample[sample_index].left_volume );
     new_sample[sample_index].right_volume = Fx_Volume( new_sample[sample_index].right_volume );

     if( new_sample[sample_index].left_volume > 5 || new_sample[sample_index].right_volume > 5 )
         {
          new_sample[sample_index].data = old_sample->data; 

          sb_mix_sample( &new_sample[sample_index] );

          if( ++sample_index >= MAX_JON_SAMPLES )
              sample_index = 0;

         } /* End if volume */

} /* End of Play_Menu_Sound */

