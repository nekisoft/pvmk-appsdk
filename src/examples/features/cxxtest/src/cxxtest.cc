//cxxtest.cc
//Tests C++ features on PVMK application SDK
//Bryan E. Topp <betopp@betopp.com> 2025

#include <vector>
#include <tuple>
#include <cstring>
#include <cstdint>
#include <cstdlib>

#include <sc.h>

std::vector<std::tuple<int, int>> Points;

uint16_t fbs[3][240][320];
int fbnext;

int main(void)
{
	for(int pp = 0; pp < 100; pp++)
	{
		Points.push_back(std::make_tuple(rand() % 320, rand() % 240));
	}
	
	while(1)
	{
		int scroll_amount = (_sc_getticks() / 64) % 240;

		memset(fbs[fbnext], 0, sizeof(fbs[fbnext]));
		for(const std::tuple<int, int> &pp : Points)
		{
			int yy = (std::get<1>(pp) + scroll_amount) % 240;
			int xx = (std::get<0>(pp) +             0) % 320;
			
			fbs[fbnext][yy][xx] = 0xFFFF;
		}
		
		int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[fbnext]);
		for(int ff = 0; ff < 3; ff++)
		{
			if(displayed == (intptr_t)(fbs[ff]))
				continue;
			if(ff == fbnext)
				continue;
			
			fbnext = ff;
			break;
		}
	}		
	
	return 0;
}
