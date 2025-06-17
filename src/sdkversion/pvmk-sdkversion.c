//pvmk-sdkversion.c
//Reports version of the PVMK SDK being used
//Bryan E. Topp <betopp@betopp.com> 2025

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Version strings we can report - mostly defined on compiler commandline
static const char *stringtab[][4] = 
{
	{ "h", "help",    "###",        "This help information"                 },
	{ "v", "version", BUILDVERSION, "Version of the SDK codebase from Git"  },
	{ "d", "date",    BUILDDATE,    "Time when the SDK was compiled"        },
	{ "c", "changed", BUILDCHANGE,  "Time of last change to SDK codebase"   },
	{ "o", "os",      BUILDHOST,    "Host operating system running the SDK" },
	{ "m", "machine", BUILDMACHINE, "Processor type running the SDK"        },
	{0}
};

//Based on options given, which strings to print
int shouldprint[sizeof(stringtab)/sizeof(stringtab[0])] = {0};

//Prints an error about a bad command-line option
void badopt_long(const char *str)
{
	fprintf(stderr, "pvmk-sdkversion: Unrecognized option \"%s\". Try -h.\n", str);
	exit(-1);
}

//Prints an error about a bad command-line option
void badopt_short(char ch)
{
	char str[3] = {'-', ch, 0};
	badopt_long(str);
}

//Prints available options
void usage(void)
{
	fprintf(stderr, "pvmk-sdkversion: Information about the PVMK Application SDK\n");
	fprintf(stderr, "(Tools for the operating system of the Neki32 Game Console)\n---\n");
	fprintf(stderr, "This program identifies the location and version of the SDK.\n");
	fprintf(stderr, "If you can run this as a shell command, your PATH is correct.\n---\n");
	fprintf(stderr, "Futher, this command can give information about the SDK:\n");
	fprintf(stderr, "Short option | Long option | What it prints\n");  
	for(int ss = 0; stringtab[ss][0] != NULL; ss++)
	{
		fprintf(stderr, " %11s   %11s   %s\n", stringtab[ss][0], stringtab[ss][1], stringtab[ss][3]);
	}
	fprintf(stderr, "---\nThis SDK contains open-source code and original work for PVMK.\n");
	fprintf(stderr, "See individual code files for more information about licensing.\n");
	fprintf(stderr, "Published by Nekisoft Pty Ltd, Australian Company Number 680 583 251.\n");
}

int main(int argc, const char **argv)
{
	//Parse args to find which strings we'll print
	for(int aa = 1; aa < argc; aa++)
	{
		const char *opt = argv[aa];
		
		if(!strcmp(opt, "--"))
		{
			//Lone double dash, no meaning
			badopt_long(opt);
		}
		else if(!strcmp(opt, "-"))
		{
			//Lone single dash, no meaning
			badopt_long(opt);
		}
		else if(opt[0] == '-' && opt[1] == '-')
		{
			//Long option
			int matched = 0;
			for(int ss = 0; stringtab[ss][0] != NULL; ss++)
			{
				if(!strcmp(opt+2, stringtab[ss][1]))
				{
					shouldprint[ss] = 1;
					matched = 1;
				}
			}
			if(!matched)
			{
				badopt_long(opt);
			}
		}
		else if(opt[0] == '-')
		{
			//Short option(s)
			for(int cc = 1; opt[cc] != '\0'; cc++)
			{
				int matched = 0;
				for(int ss = 0; stringtab[ss][0] != NULL; ss++)
				{
					if(opt[cc] == stringtab[ss][0][0])
					{
						shouldprint[ss] = 1;
						matched = 1;
					}
				}
				if(!matched)
				{
					badopt_short(opt[cc]);
				}
			}
		}
		else
		{
			//No prefix
			badopt_long(opt);
		}
	}
	
	if(shouldprint[0])
	{
		//Help
		usage();
		exit(0);
	}
	
	//Print them in a known order
	for(int ss = 0; stringtab[ss][0] != NULL; ss++)
	{
		if(shouldprint[ss])
		{
			printf("%s ", stringtab[ss][2]);
		}
	}
	printf("\n");
	
	exit(0);
}
