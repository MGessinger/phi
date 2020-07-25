#ifndef LEXER_H_
#define LEXER_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

enum Token
{
	tok_eof = -1,

	tok_def = -2,
	tok_extern = -3,

	tok_ident = -4,
	tok_number = -5,
};

int gettok (char *identStr, int strLen, int *numVal);

#endif /* LEXER_H_ */
