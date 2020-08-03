#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "llvmcontrol.h"

int yylex();
int yylex_destroy();
int yyparse();
extern FILE *yyin;

const char *version = "0.0.1";

void printUsageInfo()
{
	printf( "This is Phi v%s\n", version);
	printf( "Usage: phi [Filename]\n"
		"If Filename is -, read from stdin.\n");
}

int main (int argc, char **argv)
{
	stack *filesToParse = NULL;
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
			filesToParse = push(argv[i], 0, filesToParse);
		else
		{
			switch (argv[i][1])
			{
				case '\0':
					yyin = stdin;
					yyparse();
					break;
				case 'h':
					printUsageInfo();
					break;
			}
		}
	}
	initialiseLLVM();
	if (filesToParse == NULL)
		yyparse();
	while (filesToParse != NULL)
	{
		const char *file = pop(&filesToParse);
		yyin = fopen(file, "r");
		yyparse();
		fclose(yyin);
	}
	yylex_destroy();
	shutdownLLVM(0);
	return 0;
}
