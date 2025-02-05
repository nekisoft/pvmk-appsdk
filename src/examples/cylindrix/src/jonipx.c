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

#include "jonipx.h"
#include "gameipx.h"
#include "dosbuff.h"
#include "keys.h"

#include "types.h" 

/*
#include <dpmi.h>
#include <go32.h>
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* For swab */



/* EXTERNS */


extern long exit_loop;  /* From omega.c */
extern long game_over;



/* Globals */

/* If this is ever true...connection should be terminated
   set in pop_packet */
boolean terminate_connection = FALSE;

local_address_type local_address;

//Johnm 12/4/2001 Next line was causing a runtime error in Linux
//unsigned short socket = MY_SOCKET;
packet_type send_packet, recieve_packet;

unsigned long  address_of_callback;
unsigned short segment_of_callback;
unsigned short offset_of_callback;

/*
_go32_dpmi_registers callback_regs;
_go32_dpmi_seginfo   callback_info;
*/

unsigned long recieve_count = 0;

packet_buffer_type packet_buffer;

/* End of Globals */



void Allocate_Recieve_Callback( void ) { } 


/* Return 1 if the nodes are the same, 0 if they are not */
int Compare_Nodes( node_address_type node_one, node_address_type node_two )
    {
     int i;

     for( i = 0; i < (int)sizeof(node_address_type); i++)
         if( node_one[i] != node_two[i] )
             return(0);

     return(1);

    } /* End of Compare_Nodes() */


unsigned short Endian_Swap( unsigned short old_short )
    {
     unsigned short temp_short;
     unsigned short swap_short;
     unsigned char  *char_ptr;
     unsigned char  *char_ptr_two;

     
     temp_short = old_short;

     char_ptr     = (unsigned char *)&temp_short;
     char_ptr_two = (unsigned char *)&swap_short;

     char_ptr_two[0] = char_ptr[1];
     char_ptr_two[1] = char_ptr[0];
     
     return(swap_short);
    } /* End of Endian_Swap() */


int Init_IPX( void ) {
	return(0); //Johnm 12/1/2001
	/*
     _go32_dpmi_registers r;


     // Clear out the registers 
     memset( &r, 0, sizeof(r) );


     r.x.ax = 0x7A00;
     _go32_dpmi_simulate_int( 0x2f, &r );

     if( r.h.al != 255 )
         {
          // printf("Error in installing IPX %x \n", r.h.al ); 
          return(0);
         }
     
     Get_Local_Address(); 

     return(1);
*/
} /* End of Init_IPX() */


void Get_Local_Address(void)
    {
	/*
     _go32_dpmi_registers r;

     
     // Clear out the registers 
     memset( &r, 0, sizeof(r) );
     memset( &local_address, 0, sizeof(local_address) );


     r.x.bx = 0x0009; // Subfunction 0x0009, get internetwork address 
     r.x.es = segment_of_dos_buffer;
     r.x.si = offset_of_dos_buffer;


     Copy_Into_Dos_Buffer( &local_address, sizeof(local_address) );
     _go32_dpmi_simulate_int( 0x7a, &r );
     Copy_From_Dos_Buffer( &local_address, sizeof(local_address) );
     
     
     //printf("net  %x %x %x %x \n", local_address.net[0], local_address.net[1],local_address.net[2],local_address.net[3] );
     //printf("node %x %x %x %x %x %x \n", local_address.node_address[0], local_address.node_address[1],local_address.node_address[2],local_address.node_address[3],local_address.node_address[4],local_address.node_address[5] ); 
     */

    } /* End of Get_Local_Address() */



/*
   Open a socket... 
   longetivity   == 0x00 for open till close or terminate
                 == 0xff for open till close use for tsr
   socket_number == 0 for dynamic allocation
                 == anything else
   returns  0x0  == success
            0xfe == socket table full  
            0xff == socket already open
*/
int IPX_Open_Socket( unsigned char longetivity, unsigned short *socket_number ) {
	(void)longetivity;
	(void)socket_number;
	return(0); //Johnm 12/1/2001
} 


void IPX_Close_Socket( unsigned short *socket_number ) {
	(void)socket_number;
}


void IPX_Send_Packet( ECB_type *ecb ) {
	(void)ecb;
}


int IPX_Listen_For_Packet( ECB_type *ecb ) {
	(void)ecb;
	return(1); //Johnm 12/1/2001
} 


/* Tell driver we are idle */
void Im_Idle( void ) {
	/*
     _go32_dpmi_registers r;

     
     // Clear out the registers
     memset( &r, 0, sizeof(r) );
     

     r.x.bx = 0x000A;

     _go32_dpmi_simulate_int( 0x7A, &r );
*/    
} // End of Im_Idle() 


/* Initialize all the basic shizit we need to send packets...store the
   pointer to memory where we will put packets */
//void Init_Send_Packet( ECB_type *ecb, IPX_header_type *ipx_header, unsigned short size, unsigned short sock ) {
	/*
     int i;


     memset( ipx_header, 0, sizeof( IPX_header_type ) );
     memset( ecb, 0, sizeof(ECB_type) );
    
     ecb->socket           = Endian_Swap( sock );   // Big endian socket number 
     ecb->fragment_count   = 1;                     // Fragment count?? 
     ecb->ESR              = NULL;

     // Pointer to data fragment(ipx header) 
     ecb->fragment_data = address_of_send_buffer; 
     
     ecb->fragment_size    = sizeof(IPX_header_type) + size;
     
     for( i = 0; i < 6; i++ )
         ecb->immediate_address[i] = 0xff; // Broadcast 
    
     ipx_header->checksum    = 0xffff; // No checksum 
     ipx_header->packet_type = 0;    // Packet exchange packet 
     
     for( i = 0; i < 4; i++ )
         ipx_header->destination.net[i] = local_address.net[i]; // Send to this network 

     for( i = 0; i < 6; i++ )   
         ipx_header->destination.node_address[i] = 0xff; // Send to everybody! 

     // USE A DEFINE FOR THE WRITE SOCKET!!! 
     ipx_header->destination.socket = Endian_Swap( sock ); // Send to my socket 
     

     for( i = 0; i < 4; i++ )
         ipx_header->source.net[i] = local_address.net[i];

     for( i = 0; i < 6; i++ )   
         ipx_header->source.node_address[i] = local_address.node_address[i]; // From me 

     ipx_header->source.socket = Endian_Swap( sock );  // From my socket 

*/    
//} // End of Init_Send_Packet() 


/* Initialize all the basic shizit we need to recieve packets...store the
   pointer to memory where we will put packets */
//void Init_Recieve_Packet( ECB_type *ecb, IPX_header_type *ipx_header, unsigned short size, unsigned short sock ) {
	/*
     int error_code;


     memset( ecb, 0, sizeof(ECB_type) );
     memset( ipx_header, 0, sizeof(IPX_header_type) );

     ecb->in_use           = 0x1D;  // ?? 
     ecb->socket           = Endian_Swap( sock );
     ecb->fragment_count   = 1; 


     // Set a real mode callback for my ESR function 
     Allocate_Recieve_Callback();

     ecb->ESR              = address_of_callback;

     ecb->fragment_data    = address_of_recieve_buffer; 
     
     ecb->fragment_size    = sizeof(IPX_header_type) + size;

     Copy_Into_Recieve_Buffer( ipx_header, sizeof( IPX_header_type ) + size ); 
     Copy_Into_Ecb_Recieve_Buffer( ecb, sizeof(ECB_type) );

     if( (error_code = IPX_Listen_For_Packet(ecb)) == 0xFF )
         {
          // printf("Error in listen for packet function (in init recieve) 0x%x \n", error_code); 
          exit(0);
         }

     Copy_From_Recieve_Buffer( ipx_header, sizeof( IPX_header_type ) + size );
*/
//} /* End of Init_Recieve_Packet() */



int Init_Jonipx( void ) {
	return(1); //Johnm 12/1/2001

	/*
     int temp_int;
     int i;


     if( Init_IPX() )
          {}// printf("IPX driver Detected! \n"); 
     else
          {
           // printf("No IPX driver detected! \n"); 
           return(0);
          }

     
     // printf("Opening socket at %x!! \n", socket);
     
     
     // longetivity == 0x00 for open till close or terminate , 0xff for open till close 
     if( !(temp_int = IPX_Open_Socket( 0x00, &socket )) )
         {
          // printf("Socket opened at 0x%x (error code 0x%d \n", socket, temp_int ); 
         }
     else
         {
          // printf("Error 0x%x opening socket \n", temp_int ); 
          return(0);
         }

     for( i = 0; i < sizeof( string_type ); i++ )
         send_packet.string[i] = '\0';


     Init_Send_Packet( &send_packet.ecb, &send_packet.ipx_header, sizeof(string_type),
                          socket );

     Copy_Into_Send_Buffer( &send_packet.ipx_header, sizeof( IPX_header_type ) );
     Copy_Into_Ecb_Send_Buffer( &send_packet.ecb, sizeof( ECB_type ) );

     Init_Recieve_Packet( &recieve_packet.ecb, &recieve_packet.ipx_header,
                          sizeof(string_type), socket ); 


     packet_buffer.buffer_pos   = 0;
     packet_buffer.buffer_start = 0;


     return(1);
*/    
} /* End of Init_Jon_Ipx() */




int Jon_Get_Packet( string_type string, node_address_type source_node ) {
	(void)string;
	(void)source_node;
	return(0); //Johnm 12/1/2001
} 



int Jon_Send_Packet( string_type string, int length, node_address_type dest_node ) {
	(void)string;
	(void)length;
	(void)dest_node;
	return(0); //Johnm 12/1/2001
} 


                       
void Close_Jonipx( void ) { }


/* Is there a packet ready in the packet buffer? */
int Packet_Ready( void ) {
	return(0); //Johnm 12/1/2001
} 



/* Copy the next available packet into string */
int Pop_Packet( game_packet_type *packet ) {
	(void)packet;
	return(0); //Johnm 12/1/2001
} 



