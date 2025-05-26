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
#include <string.h>      /* need memset() */

#include "util.h"
#include "prim.h"
#include "sb_stub.h"

#include <sc.h>

extern game_configuration_type game_configuration;

//From sb_stub.c
extern int g_XResolution;
extern int g_YResolution;
//From omega.c
extern int g_bRenderingSecondPlayer;
extern int g_bRenderingFirstPlayer;

/* absolute value of a */
#ifndef ABS
	#define ABS(a)          (((a)<0) ? -(a) : (a))
#endif

/* take sign of a, either -1, 0, or 1 */
#define ZSGN(a)         (((a)<0) ? -1 : (a)>0 ? 1 : 0)

/* take binary sign of a, either -1, or 1 if >= 0 */
#ifndef SGN
	#define SGN(a)          (((a)<0) ? -1 : 0)
#endif

#define INDEX_FORWARD(Index) \
   Index = (Index + 1) % VertexList->Length;

/* Advances the index by one vertex backward through the vertex list,
   wrapping at the start of the list */

#define INDEX_BACKWARD(Index) \
   Index = (Index - 1 + VertexList->Length) % VertexList->Length;

/* Advances the index by one vertex either forward or backward through
   the vertex list, wrapping at either end of the list */

#define INDEX_MOVE(Index,Direction)                                  \
   if (Direction > 0)                                                \
      Index = (Index + 1) % VertexList->Length;                      \
   else                                                              \
      Index = (Index - 1 + VertexList->Length) % VertexList->Length;

/* Describes a single point (used for a single vertex) */
struct NewPoint {
   int x;   /* x coordinate */
   int y;   /* y coordinate */
};

/* Describes a series of points (used to store a list of vertices that
   describe a polygon; each vertex is assumed to connect to the two
   adjacent vertices, and the last vertex is assumed to connect to the
   first) */
struct PointListHeader {
   int Length;                /* # of points */
   struct NewPoint *PointPtr;   /* pointer to list of points */
};

/* Describes the beginning and ending x coordinates of a single
   horizontal line */
struct HLine {
   int xStart; /* X coordinate of leftmost pixel in line */
   int xEnd;   /* X coordinate of rightmost pixel in line */
};

/* Describes a Length-int32_t series of horizontal lines, all assumed to
   be on contiguous scan lines starting at YStart and proceeding
   downward (used to describe a scan-converted polygon to the
   low-level hardware-dependent drawing code) */
struct HLineList {
   int Length;                /* # of horizontal lines */
   int yStart;                /* Y coordinate of topmost line */
   struct HLine * HLinePtr;   /* pointer to list of horz lines */
};

int FillConvexPolygon( struct PointListHeader *VertexList, int Color,
		       int xOffset, int yOffset );

/* Globals */

/* pointer to the video buffer which holds data displayed on the screen */

//uint32_t video_buffer_storage[320*240/4];

//unsigned char *video_buffer  = (unsigned char *)video_buffer_storage; //0xD0000000L; /* vram byte ptr */

//unsigned int *word_video_buffer = (unsigned int *)video_buffer_storage; //0xD0000000L;

unsigned char *double_buffer;

/* Here is our word buffer for horizontal lines and clearing the buffer
   Words at a time */

unsigned int *word_double_buffer;

unsigned short *line_double_buffer;

unsigned char dither_matrix[2][2] = { {1, 2}, 
                                      {3, 0}  };
                                      
/* unsigned char dither_matrix[3][3] = { {0, 7, 4},
				         {8, 2, 6},
				         {3, 5, 1} }; */
extern WorldStuff world_stuff;

/* wrapper */

void fast_DB_poly_scan( Face *p, int32_t vert[][4], Window *win,
			unsigned char color )
{
	(void)win;
    struct PointListHeader plh;
    int32_t i;

    plh.Length = p->size;

    if( ( plh.PointPtr = (struct NewPoint *)malloc( sizeof( struct NewPoint ) * p->size ) ) == NULL ) {
	printf("fast_DB_poly_scan() : malloc failed!!\n");
	exit(1);
    }

    for( i = 0; i < p->size; i++ ) {
        plh.PointPtr[i].x = rounding_fixed_to_long( vert[p->index[i]][X] );
        plh.PointPtr[i].y = rounding_fixed_to_long( vert[p->index[i]][Y] );

	if( plh.PointPtr[i].x < 0 ) {
	    plh.PointPtr[i].x = 0;
	}
        else if( plh.PointPtr[i].x >= 319 ) { /* hack */
            plh.PointPtr[i].x = 320;
	}

	if( plh.PointPtr[i].y < 0 ) {
	    plh.PointPtr[i].y = 0;
	}
        else if( plh.PointPtr[i].y >= 199 ) {
            plh.PointPtr[i].y = 200;
	}
    }

    FillConvexPolygon( &plh, color, 0, 0 );

    free( plh.PointPtr );
}


void DrawHorizontalLineList(struct HLineList * HLineListPtr,
      int Color)
{
   struct HLine *HLinePtr;
   int Length, Width;
   unsigned char *ScreenPtr;

   /* Point to the start of the first scan line on which to draw */
   ScreenPtr = double_buffer + (HLineListPtr->yStart * SCREEN_WIDTH);

   /* Point to the xStart/xEnd descriptor for the first (top)
      horizontal line */
   HLinePtr = HLineListPtr->HLinePtr;
   /* Draw each horizontal line in turn, starting with the top one and
      advancing one line each time */
   Length = HLineListPtr->Length;
   while (Length-- > 0) {
      /* Draw the whole horizontal line if it has a positive width */
      if ((Width = HLinePtr->xEnd - HLinePtr->xStart + 1) > 0)
	 memset(ScreenPtr + HLinePtr->xStart, Color, Width);
      HLinePtr++;                /* point to next scan line x info */
      ScreenPtr += SCREEN_WIDTH; /* point to next scan line start */
   }
}

static void ScanEdge(int, int, int, int, int, int, struct HLine **);

int FillConvexPolygon( struct PointListHeader *VertexList, int Color,
		       int xOffset, int yOffset)
{
   int i, MinIndexL, MaxIndex, MinIndexR, SkipFirst, Temp;
   int MinPoint_y, MaxPoint_y, TopIsFlat, LeftEdgeDir;
   int NextIndex, CurrentIndex, PreviousIndex;
   int DeltaxN, DeltayN, DeltaxP, DeltayP;
   struct HLineList WorkingHLineList;
   struct HLine *EdgePointPtr;
   struct NewPoint *VertexPtr;

   /* Point to the vertex list */

   VertexPtr = VertexList->PointPtr;

   /* Scan the list to find the top and bottom of the polygon */

   if (VertexList->Length == 0)
      return(1);  /* reject null polygons */

   MaxPoint_y = MinPoint_y = VertexPtr[MinIndexL = MaxIndex = 0].y;

   for (i = 1; i < VertexList->Length; i++) {
      if (VertexPtr[i].y < MinPoint_y)
	 MinPoint_y = VertexPtr[MinIndexL = i].y; /* new top */
      else if (VertexPtr[i].y > MaxPoint_y)
	 MaxPoint_y = VertexPtr[MaxIndex = i].y; /* new bottom */
   }

   if (MinPoint_y == MaxPoint_y)
      return(1);  /* polygon is 0-height; avoid infinite loop below */

   /* Scan in ascending order to find the last top-edge point */

   MinIndexR = MinIndexL;
   while (VertexPtr[MinIndexR].y == MinPoint_y)
      INDEX_FORWARD(MinIndexR);
   INDEX_BACKWARD(MinIndexR); /* back up to last top-edge point */

   /* Now scan in descending order to find the first top-edge point */

   while (VertexPtr[MinIndexL].y == MinPoint_y)
      INDEX_BACKWARD(MinIndexL);
   INDEX_FORWARD(MinIndexL); /* back up to first top-edge point */

   /* Figure out which direction through the vertex list from the top
      vertex is the left edge and which is the right */

   LeftEdgeDir = -1; /* assume left edge runs down thru vertex list */
   if ((TopIsFlat = (VertexPtr[MinIndexL].x !=
	 VertexPtr[MinIndexR].x) ? 1 : 0) == 1) {

      /* If the top is flat, just see which of the ends is leftmost */

      if (VertexPtr[MinIndexL].x > VertexPtr[MinIndexR].x) {
	 LeftEdgeDir = 1;  /* left edge runs up through vertex list */
	 Temp = MinIndexL;       /* swap the indices so MinIndexL   */
	 MinIndexL = MinIndexR;  /* points to the start of the left */
	 MinIndexR = Temp;       /* edge, similarly for MinIndexR   */
      }
   }
   else {
      /* Point to the downward end of the first line of each of the
	 two edges down from the top */

      NextIndex = MinIndexR;
      INDEX_FORWARD(NextIndex);
      PreviousIndex = MinIndexL;
      INDEX_BACKWARD(PreviousIndex);

      /* Calculate x and y lengths from the top vertex to the end of
	 the first line down each edge; use those to compare slopes
	 and see which line is leftmost */

      DeltaxN = VertexPtr[NextIndex].x - VertexPtr[MinIndexL].x;
      DeltayN = VertexPtr[NextIndex].y - VertexPtr[MinIndexL].y;
      DeltaxP = VertexPtr[PreviousIndex].x - VertexPtr[MinIndexL].x;
      DeltayP = VertexPtr[PreviousIndex].y - VertexPtr[MinIndexL].y;

      if (((int32_t)DeltaxN * DeltayP - (int32_t)DeltayN * DeltaxP) < 0L) {
	 LeftEdgeDir = 1;  /* left edge runs up through vertex list */
	 Temp = MinIndexL;       /* swap the indices so MinIndexL   */
	 MinIndexL = MinIndexR;  /* points to the start of the left */
	 MinIndexR = Temp;       /* edge, similarly for MinIndexR   */
      }
   }

   /* Set the # of scan lines in the polygon, skipping the bottom edge
      and also skipping the top vertex if the top isn't flat because
      in that case the top vertex has a right edge component, and set
      the top scan line to draw, which is likewise the second line of
      the polygon unless the top is flat */

   if ((WorkingHLineList.Length =
	 MaxPoint_y - MinPoint_y - 1 + TopIsFlat) <= 0)
      return(1);  /* there's nothing to draw, so we're done */
   WorkingHLineList.yStart = yOffset + MinPoint_y + 1 - TopIsFlat;

   /* Get memory in which to store the line list we generate */

   if ((WorkingHLineList.HLinePtr =
	 (struct HLine *) (malloc(sizeof(struct HLine) *
	 WorkingHLineList.Length))) == NULL)
      return(0);  /* couldn't get memory for the line list */

   /* Scan the left edge and store the boundary points in the list */
   /* Initial pointer for storing scan converted left-edge coords */

   EdgePointPtr = WorkingHLineList.HLinePtr;
   /* Start from the top of the left edge */
   PreviousIndex = CurrentIndex = MinIndexL;
   /* Skip the first point of the first line unless the top is flat;
      if the top isn't flat, the top vertex is exactly on a right
      edge and isn't drawn */
   SkipFirst = TopIsFlat ? 0 : 1;
   /* Scan convert each line in the left edge from top to bottom */
   do {
      INDEX_MOVE(CurrentIndex,LeftEdgeDir);
      ScanEdge(VertexPtr[PreviousIndex].x + xOffset,
	    VertexPtr[PreviousIndex].y,
	    VertexPtr[CurrentIndex].x + xOffset,
	    VertexPtr[CurrentIndex].y, 1, SkipFirst, &EdgePointPtr);
      PreviousIndex = CurrentIndex;
      SkipFirst = 0; /* scan convert the first point from now on */
   } while (CurrentIndex != MaxIndex);

   /* Scan the right edge and store the boundary points in the list */

   EdgePointPtr = WorkingHLineList.HLinePtr;
   PreviousIndex = CurrentIndex = MinIndexR;
   SkipFirst = TopIsFlat ? 0 : 1;
   /* Scan convert the right edge, top to bottom. x coordinates are
      adjusted 1 to the left, effectively causing scan conversion of
      the nearest points to the left of but not exactly on the edge */
   do {
      INDEX_MOVE(CurrentIndex,-LeftEdgeDir);
      ScanEdge(VertexPtr[PreviousIndex].x + xOffset - 1,
	    VertexPtr[PreviousIndex].y,
	    VertexPtr[CurrentIndex].x + xOffset - 1,
	    VertexPtr[CurrentIndex].y, 0, SkipFirst, &EdgePointPtr);
      PreviousIndex = CurrentIndex;
      SkipFirst = 0; /* scan convert the first point from now on */
   } while (CurrentIndex != MaxIndex);

   /* Draw the line list representing the scan converted polygon */
   DrawHorizontalLineList(&WorkingHLineList, Color);

   /* Release the line list's memory and we're successfully done */
   free(WorkingHLineList.HLinePtr);
   return(1);
}

/* Scan converts an edge from (x1,y1) to (x2,y2), not including the
   point at (x2,y2). This avoids overlapping the end of one line with
   the start of the next, and causes the bottom scan line of the
   polygon not to be drawn. If SkipFirst != 0, the point at (x1,y1)
   isn't drawn. For each scan line, the pixel closest to the scanned
   line without being to the left of the scanned line is chosen */

void ScanEdge(int x1, int y1, int x2, int y2, int SetxStart,
      int SkipFirst, struct HLine **EdgePointPtr)
{
   int Deltax, Height, Width, AdvanceAmt, ErrorTerm, i;
   int ErrorTermAdvance, xMajorAdvanceAmt;
   struct HLine *WorkingEdgePointPtr;

   WorkingEdgePointPtr = *EdgePointPtr; /* avoid double dereference */
   AdvanceAmt = ((Deltax = x2 - x1) > 0) ? 1 : -1;
			    /* direction in which x moves (y2 is
			       always > y1, so y always counts up) */

   if ((Height = y2 - y1) <= 0)  /* y length of the edge */
      return;     /* guard against 0-length and horizontal edges */

   /* Figure out whether the edge is vertical, diagonal, x-major
      (mostly horizontal), or y-major (mostly vertical) and handle
      appropriately */
   if ((Width = abs(Deltax)) == 0) {
      /* The edge is vertical; special-case by just storing the same
	 x coordinate for every scan line */
      /* Scan the edge for each scan line in turn */
      for (i = Height - SkipFirst; i-- > 0; WorkingEdgePointPtr++) {
	 /* Store the x coordinate in the appropriate edge list */
	 if (SetxStart == 1)
	    WorkingEdgePointPtr->xStart = x1;
	 else
	    WorkingEdgePointPtr->xEnd = x1;
      }
   } else if (Width == Height) {
      /* The edge is diagonal; special-case by advancing the x
	 coordinate 1 pixel for each scan line */
      if (SkipFirst) /* skip the first point if so indicated */
	 x1 += AdvanceAmt; /* move 1 pixel to the left or right */
      /* Scan the edge for each scan line in turn */
      for (i = Height - SkipFirst; i-- > 0; WorkingEdgePointPtr++) {
	 /* Store the x coordinate in the appropriate edge list */
	 if (SetxStart == 1)
	    WorkingEdgePointPtr->xStart = x1;
	 else
	    WorkingEdgePointPtr->xEnd = x1;
	 x1 += AdvanceAmt; /* move 1 pixel to the left or right */
      }
   } else if (Height > Width) {
      /* Edge is closer to vertical than horizontal (y-major) */
      if (Deltax >= 0)
	 ErrorTerm = 0; /* initial error term going left->right */
      else
	 ErrorTerm = -Height + 1;   /* going right->left */
      if (SkipFirst) {   /* skip the first point if so indicated */
	 /* Determine whether it's time for the x coord to advance */
	 if ((ErrorTerm += Width) > 0) {
	    x1 += AdvanceAmt; /* move 1 pixel to the left or right */
	    ErrorTerm -= Height; /* advance ErrorTerm to next point */
	 }
      }
      /* Scan the edge for each scan line in turn */
      for (i = Height - SkipFirst; i-- > 0; WorkingEdgePointPtr++) {
	 /* Store the x coordinate in the appropriate edge list */
	 if (SetxStart == 1)
	    WorkingEdgePointPtr->xStart = x1;
	 else
	    WorkingEdgePointPtr->xEnd = x1;
	 /* Determine whether it's time for the x coord to advance */
	 if ((ErrorTerm += Width) > 0) {
	    x1 += AdvanceAmt; /* move 1 pixel to the left or right */
	    ErrorTerm -= Height; /* advance ErrorTerm to correspond */
	 }
      }
   } else {
      /* Edge is closer to horizontal than vertical (x-major) */
      /* Minimum distance to advance x each time */
      xMajorAdvanceAmt = (Width / Height) * AdvanceAmt;
      /* Error term advance for deciding when to advance x 1 extra */
      ErrorTermAdvance = Width % Height;
      if (Deltax >= 0)
	 ErrorTerm = 0; /* initial error term going left->right */
      else
	 ErrorTerm = -Height + 1;   /* going right->left */
      if (SkipFirst) {   /* skip the first point if so indicated */
	 x1 += xMajorAdvanceAmt;    /* move x minimum distance */
	 /* Determine whether it's time for x to advance one extra */
	 if ((ErrorTerm += ErrorTermAdvance) > 0) {
	    x1 += AdvanceAmt;       /* move x one more */
	    ErrorTerm -= Height; /* advance ErrorTerm to correspond */
	 }
      }
      /* Scan the edge for each scan line in turn */
      for (i = Height - SkipFirst; i-- > 0; WorkingEdgePointPtr++) {
	 /* Store the x coordinate in the appropriate edge list */
	 if (SetxStart == 1)
	    WorkingEdgePointPtr->xStart = x1;
	 else
	    WorkingEdgePointPtr->xEnd = x1;
	 x1 += xMajorAdvanceAmt;    /* move x minimum distance */
	 /* Determine whether it's time for x to advance one extra */
	 if ((ErrorTerm += ErrorTermAdvance) > 0) {
	    x1 += AdvanceAmt;       /* move x one more */
	    ErrorTerm -= Height; /* advance ErrorTerm to correspond */
	 }
      }
   }

   *EdgePointPtr = WorkingEdgePointPtr;   /* advance caller's ptr */
}

void Wait_For_Vsync( void ) {
}


void Set_Video_Mode( int mode ) {
	(void)mode;
} /* End of Set_Video_Mode */


/* Clear the double buffer by filling it with zeros */
void DB_Clear_Screen( void )
{
     int32_t i;
     unsigned int *ptr = word_double_buffer;

     for( i = 0; i < 800; i++ ) {
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
    }

} /* End of DB_Clear_Screen */

void draw_border(void)
{
   Edge e;
   int32_t vert[2][4];

   /* draw top edge */

   e[0] = 0;
   e[1] = 1;

   vert[0][X] = 0;
   vert[0][Y] = 0;
   vert[1][X] = 319 << MEXP;
   vert[1][Y] = 0;

   DB_draw_edge( vert, e, 0 );

   /* draw bottom edge */

   vert[0][X] = 0;
   vert[0][Y] = 199 << MEXP;
   vert[1][X] = 319 << MEXP;
   vert[1][Y] = 199 << MEXP;

   DB_draw_edge( vert, e, 0 );

   /* draw left edge */

   vert[0][X] = 0;
   vert[0][Y] = 0;
   vert[1][X] = 0;
   vert[1][Y] = 199 << MEXP;

   DB_draw_edge( vert, e, 0 );

   /* draw right edge */

   vert[0][X] = 319 << MEXP;
   vert[0][Y] = 0;
   vert[1][X] = 319 << MEXP;
   vert[1][Y] = 199 << MEXP;

   DB_draw_edge( vert, e, 0 );

}

void Fade_Screen( void )
    {
     int32_t i;

     for( i = 0; i < 64000; i++ )
	 {
	  if( double_buffer[i] >= 10 )
	      double_buffer[i] -= 10;

	  if( double_buffer[i] < 10 && double_buffer[i] != 0 )
	      double_buffer[i]--;

	 }
    }



void H_Line_Fast( int x1, int x2, int y, unsigned int color )
    {
     int i;
     int32_t line_offset;
     unsigned short middle_word;
     unsigned short first_word;
     unsigned short last_word;


     middle_word = ((color<<8) | color );

     line_offset = ((y<<7) + (y<<5));


     if( ( x1 & 0x0001) )
	 {
	  first_word = ((color << 8) | double_buffer[((y<<8) + (y<<6)) + x1]);
	 }
     else
	 {
	  first_word = ((color << 8) | color);
	 }

     if( (x2 & 0x0001 ) )
	 {
	  last_word = ((color << 8) | color);
	 }
     else
	 {
	  last_word = (color | double_buffer[((y<<8) + (y<<6)) + x2]);
	 }

     line_double_buffer[ line_offset + (x1>>1)] = first_word;
     line_double_buffer[ line_offset + (x2>>1)] = last_word;

     for( i = (x1 >> 1); i <= (x2 >> 1); i++ )
	 {
	  line_double_buffer[line_offset + i] = middle_word;
	 }



    }

/* Clear the video buffer by filling it with zeros */
void Clear_Screen( void )
    {
   //  int32_t i;
//
  //   for( i = 0; i < 64000; i++ )
//	 video_buffer[i] = 0;


   /*  _fmemset(video_buffer, 0, 320*200 );  Must make a faster one  */
    }


extern uint16_t pvmk_palette[256];
    
/* Set the palette register at index to the color trinity in color */
void Set_Palette_Register( int index, RGB_color_ptr color ) {
	
	pvmk_palette[index] = 0;
	
	pvmk_palette[index] <<= 5;
	pvmk_palette[index] |= (color->red & 0x3F) >> 1;
	
	pvmk_palette[index] <<= 6;
	pvmk_palette[index] |= (color->green & 0x3F) >> 0;
	
	pvmk_palette[index] <<= 5;
	pvmk_palette[index] |= (color->blue & 0x3F) >> 1;

} 


/* Initialize about 64k for the double buffer */
void Init_Double_Buffer( void )  /* Remember double_buffer is global */
    {


     if ( !(double_buffer = (unsigned char *)malloc( (size_t)64000 + 1)  ) )
       {
	printf("Not enough memory!!!");
	exit(0);
       }

   /* Alias our word buffer to the double buffer */

     word_double_buffer = (unsigned int *)double_buffer;
     line_double_buffer = (unsigned short *)double_buffer;

    }  /* End Init_Double_Buffer */



/* Return a pointer to this global double buffer to the caller */
/* This is for when another package needs a pointer to the */
/* Double buffer for fast access */
char *Return_Double_Buffer( void )
   {
    return( (char*)double_buffer);
   }


/* Put a value in the double buffer at x,y of value color */
void DB_Fast_Pixel( int x, int y, unsigned char color )
    {
     double_buffer[((y<<8) + (y<<6)) + x] = color;
    }

/* Put a value in the double buffer at x,y of value color, but if x or y
   is out of range then no pixel is drawn */

void DB_Scissor_Pixel( int x, int y, unsigned char color )
{
    if( (x >= 0) && (x < 320) && (y >= 0) && (y < 200) ) {
        double_buffer[((y<<8) + (y<<6)) + x] = color;
    }
}

//BITMAP *g_bmp = NULL;

/*  Copy the double buffer into the video buffer */

uint16_t pvmk_buffers[3][240][320];
int pvmk_buffer_next;

void Swap_Buffer( void ) {
	
	for(int y = 0; y < 200; y++)
	{
		for(int x = 0; x < 320; x++)
		{
			unsigned char idxcol = double_buffer[((y<<8) + (y<<6)) + x];
			pvmk_buffers[pvmk_buffer_next][y][x] = pvmk_palette[idxcol];
		}
	}
	
	int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(pvmk_buffers[pvmk_buffer_next][0][0]));
	if(displayed < 0)
		return;
	
	for(int nn = 0; nn < 3; nn++)
	{
		if(pvmk_buffer_next == nn)
			continue;
		
		if((uint16_t*)(uintptr_t)displayed == &(pvmk_buffers[nn][0][0]))
			continue;
		
		pvmk_buffer_next = nn;
		break;
	}
	
	//A hack to simulate callbacks on audio
	Hack_Update_Queue();
	
	
	/*
	
	
	
	int x,y ;

	if( !g_bmp ) {
		g_bmp = create_bitmap_ex(8, 320, 200);
	}

	//A hack to simulate callbacks on audio
	Hack_Update_Queue();

	acquire_screen();

	for( y = 0; y < 200; y++ ) {
		for( x = 0 ; x < 320; x++ ) {
			putpixel(g_bmp, x, y, double_buffer[((y<<8) + (y<<6)) + x]);
		}
	}

//	blit(g_bmp, screen, 0, 0, 0, 0, 320, 200);  // copy it to the screen
	if( game_configuration.two_player && (g_bRenderingFirstPlayer || g_bRenderingSecondPlayer) ) {
		if( g_bRenderingSecondPlayer ) 
			stretch_blit(g_bmp, screen, 0, 0, 320, 200, 0, g_YResolution/2, g_XResolution, g_YResolution/2);
		else
			stretch_blit(g_bmp, screen, 0, 0, 320, 200, 0, 0, g_XResolution, g_YResolution/2);
	}
	else {
		stretch_blit(g_bmp, screen, 0, 0, 320, 200, 0, 0, g_XResolution, g_YResolution);
	}


	release_screen();
*/

/*
    int32_t i, end;
    int index = 0;


    end = 0xA0000 + 64000;

    _farsetsel( _go32_info_block.selector_for_linear_memory);
    for( i = 0xA0000; i < end; i += 4 )
	_farnspokel(i, word_double_buffer[index++] );
*/
} /* End swap buffer */


/* copy buffer into the double buffer */
void Pop_Buffer( unsigned char *buffer )
    {
     int32_t i;

     for( i = 0; i < 64000; i++ )
	 double_buffer[i] = buffer[i];


     /*
     _asm
	 {
	  push ds
	  les di, double_buffer ;Set the destination ( video buffer )
	  lds si, buffer        ;Set the source ( double buffer )
	  mov cx, 320*200/2     ;We want to move 64000/2 words
	  rep movsw             ;Do the move
	  pop ds
	 }
    */


    }  /* End pop buffer */



/*
    Point-sampled Convex Polygon Scan Conversion and Clipping
	by Paul Heckbert
	from "Graphics Gems", Academic Press, 1990

	Modified for fixed point numbers by Anthony Thibault
*/


/* Function prototypes so they are all visible to one another */

void DB_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
		  unsigned char mask, unsigned char color );
void shade_DB_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
			unsigned char mask, unsigned char color );
void DB_zbuff_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
    unsigned char mask, unsigned char color, int32_t *zbuff );
void shade_DB_zbuff_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
    unsigned char mask, unsigned char color, int32_t *zbuff );

void inc_y4( int32_t p1[4], int32_t p2[4], int32_t p[4], int32_t dp[4], int32_t y,
	     unsigned char mask );
void inc_x4( int32_t p1[4], int32_t p2[4], int32_t p[4], int32_t dp[4], int32_t x,
	     unsigned char mask );
void increment4( int32_t p[4], int32_t dp[4], unsigned char mask );


void DB_poly_scan( Face *p, int32_t vert[][4], Window *win, unsigned char color )
/* scan converts concave polygons. */
/* p is a pointer to the face to be scan converted. */
/* vert is an array of vertices. p hold indices into this array. */
/* win is a pointer to a Window struct, it holds the clipping rectangle. */
/* color will be the color of the filled polygon. */
{
    int i, li, ri, y, ly, ry, top, rem;
    int32_t ymin; /* MAGIC */
    int32_t l[4] = {0}, r[4] = {0}, dl[4] = {0}, dr[4] = {0}; /* MAGIC */
    unsigned char mask;

    top = 0;

    ymin = JUMBO;               /* find top vertex (y points down) */
    for( i = 0; i < p->size; i++ )
	if( vert[p->index[i]][Y] < ymin ) {
	    ymin = vert[p->index[i]][Y];
	    top = i;
	}

    li = ri = top;    /* left and right vertex indices */
    rem = p->size;    /* number of vertices remaining */
    y = ymin >> MEXP; /* current scanline */
    ly = ry = y;

    mask = 0x1; /* interpolate x and z = 5, just x = 4 */

    while( rem > 0 ) {
	while( ly <= y && rem > 0 ) {
	    rem--;
	    i = li - 1;
	    if( i < 0 ) i = p->size-1;
	    inc_y4( vert[p->index[li]], vert[p->index[i]], l, dl, y, mask);
	    ly = vert[p->index[i]][Y] >> MEXP;
	    li = i;
	}

	while( ry <= y && rem > 0 ) {
	    rem--;
	    i = ri + 1;
	    if( i >= p->size ) i = 0;
	    inc_y4( vert[p->index[ri]], vert[p->index[i]], r, dr, y, mask);
	    ry = vert[p->index[i]][Y] >> MEXP;
	    ri = i;
	}

	while( y < ly && y < ry ) {
	    if( y >= win->y0 && y <= win->y1 ) {
		    if( l[X] <= r[X] )
			DB_scanline( y, l, r, win, mask, color );
		    else
			DB_scanline( y, r, l, win, mask, color );
	    }
	    y++;
	    increment4( l, dl, mask );
	    increment4( r, dr, mask );
	}
    }
}

void DB_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
		  unsigned char mask, unsigned char color )
{
    int32_t lx, rx;
    int32_t p[4], dp[4]; /* MAGIC */

    mask &= ~0x1; /* stop interpolating x */

    lx = rounding_fixed_to_long( l[X] );
    if( lx < win->x0 ) lx = win->x0;
    rx = rounding_fixed_to_long( r[X] );
    if( rx > win->x1 ) rx = win->x1;

    if( lx > rx ) return;
    inc_x4( l, r, p, dp, lx, mask );

    memset( (char *)(double_buffer + ((y<<8) + (y<<6)) + lx), color, rx-lx+1 );

}

void shade_DB_poly_scan( Face *p, int32_t vert[][4], Window *win,
			 unsigned char color )
/* scan converts concave polygons. */
/* p is a pointer to the face to be scan converted. */
/* vert is an array of vertices. p hold indices into this array. */
/* win is a pointer to a Window struct, it holds the clipping rectangle. */
/* color will be the color of the filled polygon. */
{
    int i, li, ri, y, ly, ry, top, rem;
    int32_t ymin; /* MAGIC */
    int32_t l[4], r[4], dl[4], dr[4]; /* MAGIC */
    unsigned char mask;

    top = 0;

    mask = 0x09;

    ymin = JUMBO;               /* find top vertex (y points down) */
    for( i = 0; i < p->size; i++ )
	if( vert[p->index[i]][Y] < ymin ) {
	    ymin = vert[p->index[i]][Y];
	    top = i;
	}

    li = ri = top;  /* left and right vertex indices */
    rem = p->size;  /* number of vertices remaining */
    y = ymin >> MEXP; /* current scanline */
    ly = ry = y;

    while( rem > 0 ) {
	while( ly <= y && rem > 0 ) {
	    rem--;
	    i = li - 1;
	    if( i < 0 ) i = p->size-1;
	    inc_y4( vert[p->index[li]], vert[p->index[i]], l, dl, y, mask);
	    ly = vert[p->index[i]][Y] >> MEXP;
	    li = i;
	}

	while( ry <= y && rem > 0 ) {
	    rem--;
	    i = ri + 1;
	    if( i >= p->size ) i = 0;
	    inc_y4( vert[p->index[ri]], vert[p->index[i]], r, dr, y, mask);
	    ry = vert[p->index[i]][Y] >> MEXP;
	    ri = i;
	}

	while( y < ly && y < ry ) {
	    if( y >= win->y0 && y <= win->y1 ) {
		if( l[X] <= r[X] )
		    shade_DB_scanline( y, l, r, win, mask, color );
		else
		    shade_DB_scanline( y, r, l, win, mask, color );
	    }
	    y++;
	    increment4( l, dl, mask );
	    increment4( r, dr, mask );
	}
    }
}

void shade_DB_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
			unsigned char mask, unsigned char color )
{
	(void)color;
    int32_t x, lx, rx;
    int32_t p[4], dp[4]; /* MAGIC */
    unsigned char *db_ptr;

    mask &= ~0x1; /* stop interpolating x */

    lx = l[X] >> MEXP;

    if( lx < win->x0 ) lx = win->x0;

    rx = r[X] >> MEXP;

    if( rx > win->x1 ) rx = win->x1;
    if( lx > rx ) return;
    inc_x4( l, r, p, dp, lx, mask );

    db_ptr = &double_buffer[ ((y<<8) + (y<<6)) + lx ];

    for( x = lx; x < rx; x++ ) {
    
        if( (dither_matrix[x%2][y%2]) < ((p[3] >> 8) & 3) ) { /* home */
            *db_ptr = (unsigned char)(p[3] >> 10) + 1;
	    db_ptr++;
	    increment4( p, dp, mask );
	}
	else {
	    *db_ptr = (unsigned char)(p[3] >> 10);
	    db_ptr++;
	    increment4( p, dp, mask );
	}
    }
}

void DB_zbuff_poly_scan( Face *p, int32_t vert[][4], Window *win, unsigned char color, int32_t *zbuff )
/* scan converts concave polygons. */
/* p is a pointer to the face to be scan converted. */
/* vert is an array of vertices. p hold indices into this array. */
/* win is a pointer to a Window struct, it holds the clipping rectangle. */
/* color will be the color of the filled polygon. */
{
    int i, li, ri, y, ly, ry, top, rem;
    int32_t ymin; /* MAGIC */
    int32_t l[4], r[4], dl[4], dr[4]; /* MAGIC */
    unsigned char mask;

    top = 0;

    ymin = JUMBO;               /* find top vertex (y points down) */
    for( i = 0; i < p->size; i++ )
	if( vert[p->index[i]][Y] < ymin ) {
	    ymin = vert[p->index[i]][Y];
	    top = i;
	}

    li = ri = top;    /* left and right vertex indices */
    rem = p->size;    /* number of vertices remaining */
    y = ymin >> MEXP; /* current scanline */
    ly = ry = y;

    mask = 0x5; /* interpolate x and z = 5, just x = 1 */

    while( rem > 0 ) {
	while( ly <= y && rem > 0 ) {
	    rem--;
	    i = li - 1;
	    if( i < 0 ) i = p->size-1;
	    inc_y4( vert[p->index[li]], vert[p->index[i]], l, dl, y, mask);
	    ly = vert[p->index[i]][Y] >> MEXP;
	    li = i;
	}

	while( ry <= y && rem > 0 ) {
	    rem--;
	    i = ri + 1;
	    if( i >= p->size ) i = 0;
	    inc_y4( vert[p->index[ri]], vert[p->index[i]], r, dr, y, mask);
	    ry = vert[p->index[i]][Y] >> MEXP;
	    ri = i;
	}

	while( y < ly && y < ry ) {
	    if( y >= win->y0 && y <= win->y1 ) {
		    if( l[X] <= r[X] )
			DB_zbuff_scanline( y, l, r, win, mask, color, zbuff );
		    else
			DB_zbuff_scanline( y, r, l, win, mask, color, zbuff );
	    }
	    y++;
	    increment4( l, dl, mask );
	    increment4( r, dr, mask );
	}
    }
}

void DB_zbuff_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
			unsigned char mask, unsigned char color, int32_t *zbuff )
{
    int32_t x, lx, rx;
    int32_t p[4], dp[4]; /* MAGIC */
    int32_t *zb_ptr;
    unsigned char *db_ptr;

    mask &= ~0x1; /* stop interpolating x */

    lx = l[X] >> MEXP;
    if( lx < win->x0 ) lx = win->x0;
	rx = r[X] >> MEXP;

    if( rx > win->x1 ) rx = win->x1;
    if( lx > rx ) return;
    inc_x4( l, r, p, dp, lx, mask );

    zb_ptr = &zbuff[ ((y<<8) + (y<<6)) + lx ];
    db_ptr = &double_buffer[ ((y<<8) + (y<<6)) + lx ];

    for( x = lx; x < rx; x++ ) {
	if( p[Z] < *zb_ptr ) {
	    *db_ptr = color;  /* plot the pixel */
	    *zb_ptr = p[Z];   /* set the zbuffer */
	}
	zb_ptr++;
	db_ptr++;

	increment4( p, dp, mask );
    }
}

void shade_DB_zbuff_poly_scan( Face *p, int32_t vert[][4], Window *win,
			       unsigned char color, int32_t *zbuff )
/* scan converts concave polygons. */
/* p is a pointer to the face to be scan converted. */
/* vert is an array of vertices. p hold indices into this array. */
/* win is a pointer to a Window struct, it holds the clipping rectangle. */
/* color will be the color of the filled polygon. */
{
    int i, li, ri, y, ly, ry, top, rem;
    int32_t ymin; /* MAGIC */
    int32_t l[4], r[4], dl[4], dr[4]; /* MAGIC */
    unsigned char mask;

    top = 0;

    ymin = JUMBO;               /* find top vertex (y points down) */
    for( i = 0; i < p->size; i++ )
	if( vert[p->index[i]][Y] < ymin ) {
	    ymin = vert[p->index[i]][Y];
	    top = i;
	}

    li = ri = top;    /* left and right vertex indices */
    rem = p->size;    /* number of vertices remaining */
    y = ymin >> MEXP; /* current scanline */
    ly = ry = y;

    mask = 0xd; /* interpolate x and z = 5, just x = 1 */

    while( rem > 0 ) {
	while( ly <= y && rem > 0 ) {
	    rem--;
	    i = li - 1;
	    if( i < 0 ) i = p->size-1;
	    inc_y4( vert[p->index[li]], vert[p->index[i]], l, dl, y, mask);
	    ly = vert[p->index[i]][Y] >> MEXP;
	    li = i;
	}

	while( ry <= y && rem > 0 ) {
	    rem--;
	    i = ri + 1;
	    if( i >= p->size ) i = 0;
	    inc_y4( vert[p->index[ri]], vert[p->index[i]], r, dr, y, mask);
	    ry = vert[p->index[i]][Y] >> MEXP;
	    ri = i;
	}

	while( y < ly && y < ry ) {
	    if( y >= win->y0 && y <= win->y1 ) {
		    if( l[X] <= r[X] )
			shade_DB_zbuff_scanline( y, l, r, win, mask, color, zbuff );
		    else
			shade_DB_zbuff_scanline( y, r, l, win, mask, color, zbuff );
	    }
	    y++;
	    increment4( l, dl, mask );
	    increment4( r, dr, mask );
	}
    }
}

void shade_DB_zbuff_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
			unsigned char mask, unsigned char color, int32_t *zbuff )
{
	(void)color;
    int32_t x, lx, rx;
    int32_t p[4], dp[4]; /* MAGIC */
    int32_t *zb_ptr;
    unsigned char *db_ptr;

    mask &= ~0x1; /* stop interpolating x */

    lx = l[X] >> MEXP;
    if( lx < win->x0 ) lx = win->x0;
	rx = r[X] >> MEXP;

    if( rx > win->x1 ) rx = win->x1;
    if( lx > rx ) return;
    inc_x4( l, r, p, dp, lx, mask );

    zb_ptr = &zbuff[ ((y<<8) + (y<<6)) + lx ];
    db_ptr = &double_buffer[ ((y<<8) + (y<<6)) + lx ];

    for( x = lx; x < rx; x++ ) {
	if( p[Z] < *zb_ptr ) {
	    *db_ptr = (unsigned char)(p[3] >> MEXP);
	    *zb_ptr = p[Z];
	}
	db_ptr++;
	zb_ptr++;
	increment4( p, dp, mask );
    }
}

void DB_transparent_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
		              unsigned char mask, unsigned char color );

void DB_transparent_poly_scan( Face *p, int32_t vert[][4], Window *win, unsigned char color )

/* scan converts concave polygons. */
/* p is a pointer to the face to be scan converted. */
/* vert is an array of vertices. p hold indices into this array. */
/* win is a pointer to a Window struct, it holds the clipping rectangle. */
/* color will be the color of the filled polygon. */
{
    int i, li, ri, y, ly, ry, top, rem;
    int32_t ymin; /* MAGIC */
    int32_t l[4] = {0}, r[4] = {0}, dl[4] = {0}, dr[4] = {0}; /* MAGIC */
    unsigned char mask;

    top = 0;

    ymin = JUMBO;               /* find top vertex (y points down) */
    for( i = 0; i < p->size; i++ )
	if( vert[p->index[i]][Y] < ymin ) {
	    ymin = vert[p->index[i]][Y];
	    top = i;
	}

    li = ri = top;    /* left and right vertex indices */
    rem = p->size;    /* number of vertices remaining */
    y = ymin >> MEXP; /* current scanline */
    ly = ry = y;

    mask = 0x1; /* interpolate x and z = 5, just x = 4 */

    while( rem > 0 ) {
	while( ly <= y && rem > 0 ) {
	    rem--;
	    i = li - 1;
	    if( i < 0 ) i = p->size-1;
	    inc_y4( vert[p->index[li]], vert[p->index[i]], l, dl, y, mask);
	    ly = vert[p->index[i]][Y] >> MEXP;
	    li = i;
	}

	while( ry <= y && rem > 0 ) {
	    rem--;
	    i = ri + 1;
	    if( i >= p->size ) i = 0;
	    inc_y4( vert[p->index[ri]], vert[p->index[i]], r, dr, y, mask);
	    ry = vert[p->index[i]][Y] >> MEXP;
	    ri = i;
	}

	while( y < ly && y < ry ) {
	    if( y >= win->y0 && y <= win->y1 ) {
		    if( l[X] <= r[X] ) 
			DB_transparent_scanline( y, l, r, win, mask, color );
		    else
			DB_transparent_scanline( y, r, l, win, mask, color );
	    }
	    y++;
	    increment4( l, dl, mask );
	    increment4( r, dr, mask );
	}
    }
}

void DB_transparent_scanline( int32_t y, int32_t l[4], int32_t r[4], Window *win,
		              unsigned char mask, unsigned char color )
{
	(void)color;
    int32_t lx, rx;
    int32_t p[4], dp[4]; /* MAGIC */
    int32_t new_color, old_color;
    int32_t x;
    int32_t i;
    unsigned char *db_ptr;

    mask &= ~0x1; /* stop interpolating x */

    lx = rounding_fixed_to_long( l[X] );
    if( lx < win->x0 ) lx = win->x0;
    rx = rounding_fixed_to_long( r[X] );
    if( rx > win->x1 ) rx = win->x1;

    if( lx > rx ) return;
    inc_x4( l, r, p, dp, lx, mask );
    
    db_ptr = &double_buffer[ ((y<<8) + (y<<6)) + lx ];
    
    new_color = 0;
    
    for( x = lx; x <= rx; x++ ) {
        old_color = *db_ptr;
        
        for( i = 0; i < world_stuff.color_info.size; i++ ) {
            if( old_color == 0 ) {
            
                new_color = world_stuff.color_info.gradient[TubeFirstGrad].first +
                            (((world_stuff.color_info.gradient[TubeFirstGrad].num_colors << MEXP) / 32) >> MEXP);
                            
                if( new_color > world_stuff.color_info.gradient[TubeFirstGrad].last ) {
                    new_color = world_stuff.color_info.gradient[TubeFirstGrad].last;
                }
            }
            else if( (old_color >= world_stuff.color_info.gradient[i].first) &&
                (old_color <= world_stuff.color_info.gradient[i].last) ) {
                
                new_color = old_color + (((world_stuff.color_info.gradient[i].num_colors << MEXP) / 32) >> MEXP);
                
                if( new_color > world_stuff.color_info.gradient[i].last ) {
                    new_color = world_stuff.color_info.gradient[i].last;
                }
            }
        }
        
        *db_ptr = new_color;
        db_ptr++;
    }
}


void inc_y4( int32_t p1[4], int32_t p2[4], int32_t p[4], int32_t dp[4], int32_t y,
	     unsigned char mask )
{
    int32_t dy, frac; /* MAGIC */

    dy = p2[Y] - p1[Y];
    if( dy == 0 ) dy = MAGIC;
    frac = (y << MEXP) - p1[Y] + MAGIC;

    if( mask & 0x1 ) {
	dp[X] = ((p2[X] - p1[X]) << MEXP) / dy;
	p[X] = p1[X] + ((dp[X] * frac) >> MEXP);
    }
    if( mask & 0x2 ) {
	dp[Y] = ((p2[Y] - p1[Y]) << MEXP) / dy;
	p[Y] = p1[Y] + ((dp[Y] * frac) >> MEXP);
    }
    if( mask & 0x4 ) {
	dp[Z] = ((p2[Z] - p1[Z]) << MEXP) / dy;
	p[Z] = p1[Z] + ((dp[Z] * frac) >> MEXP);
    }
    if( mask & 0x8 ) {
	dp[3] = ((p2[3] - p1[3]) << MEXP) / dy;
	p[3] = p1[3] + ((dp[3] * frac) >> MEXP);
    }
}

void inc_x4( int32_t p1[4], int32_t p2[4], int32_t p[4], int32_t dp[4], int32_t x,
	    unsigned char mask )
{
    int32_t dx, frac; /* MAGIC */

    dx = p2[X] - p1[X];
    if( dx == 0 ) dx = MAGIC;
    frac = (x << MEXP) - p1[X] + MAGIC;

    if( mask & 0x1 ) {
	dp[X] = ((p2[X] - p1[X]) << MEXP) / dx;
	p[X] = p1[X] + ((dp[X] * frac) >> MEXP);
    }
    if( mask & 0x2 ) {
	dp[Y] = ((p2[Y] - p1[Y]) << MEXP) / dx;
	p[Y] = p1[Y] + ((dp[Y] * frac) >> MEXP);
    }
    if( mask & 0x4 ) {
	dp[Z] = ((p2[Z] - p1[Z]) << MEXP) / dx;
	p[Z] = p1[Z] + ((dp[Z] * frac) >> MEXP);
    }
    if( mask & 0x8 ) {
	dp[3] = ((p2[3] - p1[3]) << MEXP) / dx;
	p[3] = p1[3] + ((dp[3] * frac) >> MEXP);
    }
}

void increment4( int32_t p[4], int32_t dp[4], unsigned char mask )
{
    if( mask & 0x1 ) {
	p[X] += dp[X];
    }
    if( mask & 0x2 ) {
	p[Y] += dp[Y];
    }
    if( mask & 0x4 ) {
	p[Z] += dp[Z];
    }
    if( mask & 0x8 ) {
	p[3] += dp[3];
    }
}


void inc_y( int32_t p1[3], int32_t p2[3], int32_t p[3], int32_t dp[3], int32_t y,
	    unsigned char mask )
{
    int32_t dy, frac; /* MAGIC */

    dy = p2[Y] - p1[Y];
    if( dy == 0 ) dy = MAGIC;
    frac = (y << MEXP) - p1[Y] + MAGIC;

    if( mask & 0x1 ) {
	dp[X] = ((p2[X] - p1[X]) << MEXP) / dy;
	p[X] = p1[X] + ((dp[X] * frac) >> MEXP);
    }
    if( mask & 0x2 ) {
	dp[Y] = ((p2[Y] - p1[Y]) << MEXP) / dy;
	p[Y] = p1[Y] + ((dp[Y] * frac) >> MEXP);
    }
}


void inc_x( int32_t p1[3], int32_t p2[3], int32_t p[3], int32_t dp[3], int32_t x,
	    unsigned char mask )
{
    int32_t dx, frac; /* MAGIC */

    dx = p2[X] - p1[X];
    if( dx == 0 ) dx = MAGIC;
    frac = (x << MEXP) - p1[X] + MAGIC;

    if( mask & 0x1 ) {
	dp[X] = ((p2[X] - p1[X]) << MEXP) / dx;
	p[X] = p1[X] + ((dp[X] * frac) >> MEXP);
    }
    if( mask & 0x2 ) {
	dp[Y] = ((p2[Y] - p1[Y]) << MEXP) / dx;
	p[Y] = p1[Y] + ((dp[Y] * frac) >> MEXP);
    }
}

void increment( int32_t p[3], int32_t dp[3], unsigned char mask )
{
    if( mask & 0x1 ) {
	p[X] += dp[X];
    }
    if( mask & 0x2 ) {
	p[Y] += dp[Y];
    }
}

/* draws a line into the double buffer */

void DB_draw_edge( int32_t vert[][4], Edge e, unsigned char color )
{
    int d, x, y, ax, ay, sx, sy, dx, dy, x1, y1, x2, y2;

    x1 = rounding_fixed_to_long( vert[e[0]][X] );

    if( x1 < 0 ) {
	x1 = 0;
    }
    else if( x1 > 319 ) {
	x1 = 319;
    }

    y1 = rounding_fixed_to_long( vert[e[0]][Y] );

    if( y1 < 0 ) {
	y1 = 0;
    }
    else if( y1 > 199 ) {
	y1 = 199;
    }

    x2 = rounding_fixed_to_long( vert[e[1]][X] );

    if( x2 < 0 ) {
	x2 = 0;
    }
    else if( x2 > 319 ) {
	x2 = 319;
    }

    y2 = rounding_fixed_to_long( vert[e[1]][Y] );

    if( y2 < 0 ) {
	y2 = 0;
    }
    else if( y2 > 199 ) {
	y2 = 199;
    }

    dx = x2-x1; ax = ABS(dx)<<1; sx = ZSGN(dx);
    dy = y2-y1; ay = ABS(dy)<<1; sy = ZSGN(dy);

    x = x1;
    y = y1;

    if( ax > ay ) {
	d = ay - (ax>>1);
	for(;;) {
	    DB_Fast_Pixel( x, y, color );
	    if( x==x2) {
		return;
	    }
	    if(d>=0) {
		y += sy;
		d -= ax;
	    }
	    d += ay;
	    x += sx;
	}
    }
    else {
	d = ax-(ay>>1);
	for(;;) {
	    DB_Fast_Pixel( x, y, color );
	    if(y==y2) {
		return;
	    }
	    if(d>=0) {
		x += sx;
		d -= ay;
	    }
	    y += sy;
	    d += ax;
	}
    }
}
