#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "ast.h"
#include "codegen.h"

int yylex();
int yylex_destroy();
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
	argv[0] = "";
	initialiseLLVM();
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
		{
			yyin = fopen(argv[i], "r");
			yyparse();
		}
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
	yylex_destroy();
	shutdownLLVM();
	return 0;
}
