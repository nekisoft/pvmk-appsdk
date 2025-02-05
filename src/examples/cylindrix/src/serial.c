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

#include "serial.h"
#include "packets.h"
#include "keys.h"     /* For jon_kbhit and jon_getkey() */
#include "sb_stub.h"
//#include <dpmi.h>
//#include <go32.h>


/* This is the number of timer clicks we will wait for remote information before raising an error */
#define NUM_CLICKS_TILL_DISCONNECT 700

/* G L O B A L S ///////////////////////////////////////////////////////////// */


extern unsigned long absolute_time; /* From timer.c */

char ser_buffer[SERIAL_BUFF_SIZE];  /* the receive buffer */

short ser_end = -1,ser_start=-1;      /* indexes into receive buffer */
short ser_ch, char_ready=0;           /* current character and ready flag */
short old_int_mask;                   /* the old interrupt mask on the PIC */
short open_port;                      /* the currently open port */
short serial_lock = 0;                /* serial ISR semaphore so the buffer */
                                    /* isn't altered will it is being written */
                                    /* to by the ISR */


/* pointer to old serial interrupt function */

// _go32_dpmi_seginfo Old_Serial_Isr, New_Serial_Isr, Old_Prot_Serial_Isr, New_Prot_Serial_Isr;   ;   

// _go32_dpmi_registers regs;      

/* END OF NEW */


/* Don't call if interrupt is active */
void Flush_Buffer( void )  {
	/*
     int i;

     for( i = 0; i < 1000; i++ )
         inportb(open_port + SER_RBF); 
         
     // outportb( PIC_ICR, 0x20 ); 
	 */
}



/* //////////////////////////////////////////////////////////////////////////// */

/*
void Serial_Isr( _go32_dpmi_registers *r ) {
// this is the ISR (Interrupt Service Routine) for the com port.  It is very 
// simple.  When it gets called, it gets the next character out of the receive 
// buffer register 0 and places it into the software buffer. Note: C takes care 
// of all the register saving and house work.  Cool huh! 

// lock out any other functions so the buffer doesn't get corrupted 
serial_lock = 1;


// place character into next position in buffer 
// ser_ch = _inp(open_port + SER_RBF); 
 // ser_ch = inportw(open_port + SER_RBF);  
 ser_ch = inportb(open_port + SER_RBF); 

// wrap buffer index around 
if (++ser_end > SERIAL_BUFF_SIZE-1)
    ser_end = 0;

// move character into buffer 
ser_buffer[ser_end] = ser_ch;

++char_ready;

// restore PIC 
// _outp(PIC_ICR,0x20); 
  outportb( PIC_ICR, 0x20 );

// undo lock 
serial_lock = 0;

} // end Serial_Isr 
*/


/* //////////////////////////////////////////////////////////////////////////// */

short Ready_Serial(void)
{

/* this functions returns true if there are any characters waiting and 0 if */
/* the buffer is empty */

return(char_ready);

} /* end Ready_Serial */

/* ///////////////////////////////////////////////////////////////////////////// */

short Serial_Read( char *send_char )
{

/* this function reads a character from the circulating buffer and returns it */
/* to the caller */

short ch;

/* wait for isr to end */

while(serial_lock){}

/* test if there is a character(s) ready in buffer */

if (ser_end != ser_start)
   {

   /* wrap buffer index if needed */

   if (++ser_start > SERIAL_BUFF_SIZE-1)
       ser_start = 0;

   /* get the character out of buffer */

   ch = ser_buffer[ser_start];

   /* one less character in buffer now */

   if (char_ready > 0)
       --char_ready;

   /* send data back to caller */

   *send_char = ch;
   return( 1 );

   } /* end if a character is in buffer */
else
   /* buffer was empty return a NULL i.e. 0 */
   return( -1 );

} /* end Serial_read */


/* ///////////////////////////////////////////////////////////////////////////// */

void Serial_Write(char ch) {
	(void)ch;
} /* end Serial_Write */

/* ///////////////////////////////////////////////////////////////////////////// */

void Open_Serial(short port_base, short baud, short configuration) {
	(void)port_base; (void)baud; (void)configuration;
} /* Open_Serial */

/* ///////////////////////////////////////////////////////////////////////////// */


void Close_Serial(short port_base) {
	(void)port_base;
} /* end Close_Serial */

/* //////////////////////////////////////////////////////////////////////////// */

/* Handshake waits to see if there is a connection, if connection is 
   made it returns 1 for master 0 for slave...if user hits escape it
   returns -1 for no connection made */

int Handshake( void )
    {
     char test_char;
     int i;
     int master = 0;


     for( i = 0; i < 100; i++ ) /* Flush the buffer */
         Serial_Read( &test_char );

     delay( 1000 );

     if( Serial_Read( &test_char ) == -1 )
         {
          /* printf("We are the master \n"); */
          master = 1;

          while( Serial_Read( &test_char ) == -1 )
              {
               if( Jon_Kbhit() )  /* If user hits escape return error */
                   {
                    if( Jon_Getkey() == INDEX_ESC )
                       return -1;

                   }

               Serial_Write( '1' );
               delay( 500 );
              }

          for( i = 0; i < 100; i++ ) /* Flush the buffer */
              {
               delay(1);
               Serial_Read( &test_char );
              }
          delay( 500 );
         }
     else
         {
          /* printf("We are the slave \n");         */
          
          Serial_Write( '2' );
          
          for( i = 0; i < 100; i++ ) /* Flush the buffer */
              {
               delay(1);
               Serial_Read( &test_char );
              }
         }
    
     return master;
    
    }  /* End of Handshake() */


/* Get one character from the remote computer...if it takes long enough...
   return 0 */
int Get_One_Character( char *ch )
     {
	     (void)ch;
	     return 0;

     } /* End of Get_One_Character() */
