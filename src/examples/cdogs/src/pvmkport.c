//pvmkport.c
//Port of CDogs to Neki32
//Bryan E. Topp <betopp@betopp.com> 2025

#include "pvmkport.h"
#include <sc.h>

void delay(int ms)
{
	int begin = _sc_getticks();
	while(_sc_getticks() < begin + ms)
	{
		_sc_pause();
	}
	return;
}

int kbhit(void)
{
	return 0;
}
