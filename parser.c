#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

void* logError(const char *errstr, int errcode)
{
	fprintf(stderr, "Error %i: %s\n", errcode, errstr);
	return NULL;
}

void* parseNumberExpr (token *curtok)
{
	NumExpr *ne = malloc(sizeof(NumExpr));
	if (ne == NULL)
		return logError("Could not allocate Memory.", 0x100);
	ne->val = curtok->numVal;
	gettok(curtok);
	return ne;
}
