#include "parser.h"
#include "lexer.h"

void* logError(const char *errstr, int errcode)
{
	fprintf(stderr, "Error %i: %s\n", errcode, errstr);
	return NULL;
}

void* parseNumberExpr (token *curtok)
{
	NumExpr *ne = malloc(sizeof(NumExpr));
	if (ne == NULL)
		return NULL;
	ne->val = curtok->numVal;
	gettok(curtok);
	return ne;
}
