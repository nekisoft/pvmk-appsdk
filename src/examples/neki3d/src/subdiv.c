//subdiv.c
//Dynamic subdivision and clipping
//Bryan E. Topp <betopp@betopp.com> 2024

#include "subdiv.h"
#include "span.h"
#include "fb.h"

static int subdiv_level = 0;

void subdiv_tri(int tex,
	const fix24p8_t *va4, const int *ta2, 
	const fix24p8_t *vb4, const int *tb2,
	const fix24p8_t *vc4, const int *tc2)
{
	//Note that this subdivision happens in homogenous space, before the perspective divide.
	//This means more divisions and therefore more expense - but is perspective-correct.
	
	if(subdiv_level == 0)
	{
		if( ((vb4[0] - va4[0]) * (vc4[1] - va4[1])) < ((vb4[1] - va4[1]) * (vc4[0] - va4[0])) )
			return; //backface
	}
	
	if(subdiv_level < 4)
	{
		int thresh2 = 8192;
		
		int x_ab = ((int64_t)(FBX) * vb4[0] / vb4[3]) - ((int64_t)(FBX) * va4[0] / va4[3]);
		int y_ab = ((int64_t)(FBY) * vb4[1] / vb4[3]) - ((int64_t)(FBY) * va4[1] / va4[3]);
		int d2_ab = (x_ab * x_ab) + (y_ab * y_ab);
		
		int x_ac = ((int64_t)(FBX) * vc4[0] / vc4[3]) - ((int64_t)(FBX) * va4[0] / va4[3]);
		int y_ac = ((int64_t)(FBY) * vc4[1] / vc4[3]) - ((int64_t)(FBY) * va4[1] / va4[3]);
		int d2_ac = (x_ac * x_ac) + (y_ac * y_ac);

		int x_bc = ((int64_t)(FBX) * vc4[0] / vc4[3]) - ((int64_t)(FBX) * vb4[0] / vb4[3]);
		int y_bc = ((int64_t)(FBY) * vc4[1] / vc4[3]) - ((int64_t)(FBY) * vb4[1] / vb4[3]);
		int d2_bc = (x_bc * x_bc) + (y_bc * y_bc);

		if(d2_ab > d2_bc && d2_ab > d2_ac && d2_ab > thresh2)
		{
			//Subdivide between vertexes A and B
			const fix24p8_t mid4[4] =
			{
				(va4[0] + vb4[0]) / 2,
				(va4[1] + vb4[1]) / 2,
				(va4[2] + vb4[2]) / 2,
				(va4[3] + vb4[3]) / 2,
			};
			
			const int tmid[2] =
			{
				(ta2[0] + tb2[0]) / 2,
				(ta2[1] + tb2[1]) / 2,
			};
			
			subdiv_level++;
			subdiv_tri(tex,  va4,  ta2, mid4, tmid, vc4, tc2);
			subdiv_tri(tex, mid4, tmid,  vb4,  tb2, vc4, tc2);
			subdiv_level--;
			return;
		}
		
		if(d2_ac > d2_ab && d2_ac > d2_bc && d2_ac > thresh2)
		{
			//Subdivide between vertexes A and C
			const fix24p8_t mid4[4] =
			{
				(va4[0] + vc4[0]) / 2,
				(va4[1] + vc4[1]) / 2,
				(va4[2] + vc4[2]) / 2,
				(va4[3] + vc4[3]) / 2,
			};
			
			const int tmid[2] =
			{
				(ta2[0] + tc2[0]) / 2,
				(ta2[1] + tc2[1]) / 2,
			};
			
			subdiv_level++;
			subdiv_tri(tex,  va4,  ta2, vb4, tb2, mid4, tmid);
			subdiv_tri(tex, mid4, tmid, vb4, tb2,  vc4,  tc2);
			subdiv_level--;
			return;
		}	
		
		if(d2_bc > d2_ac && d2_bc > d2_ab && d2_bc > thresh2)
		{
			//Subdivide between vertexes B and C
			const fix24p8_t mid4[4] =
			{
				(vb4[0] + vc4[0]) / 2,
				(vb4[1] + vc4[1]) / 2,
				(vb4[2] + vc4[2]) / 2,
				(vb4[3] + vc4[3]) / 2,
			};
			
			const int tmid[2] =
			{
				(tb2[0] + tc2[0]) / 2,
				(tb2[1] + tc2[1]) / 2,
			};
			
			subdiv_level++;
			subdiv_tri(tex, va4, ta2, mid4, tmid,  vc4,  tc2);
			subdiv_tri(tex, va4, ta2,  vb4,  tb2, mid4, tmid);
			subdiv_level--;
			return;
		}
	}
	
	//Already small enough
	span_add_tri(tex, va4, ta2, vb4, tb2, vc4, tc2);
}
