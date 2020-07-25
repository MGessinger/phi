#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"

token *makeToken (int len)
{
	token *tk = malloc(sizeof(token));
	if (tk == NULL)
		return NULL;
	tk->identStr = malloc(len*sizeof(char));
	tk->strLen = len;
	if (tk->identStr == NULL)
	{
		free(tk);
		return NULL;
	}
	tk->numVal = 0;
	tk->tok_type = 0;
	return tk;
}

void clearToken (token *tk)
{
	if (tk == NULL)
		return;
	free(tk->identStr);
	free(tk);
}

int gettok()
{
	static char lastChar = ' ';
	int tok_type = 0;
	while (isspace(lastChar))
		lastChar = getchar();
	if (isalpha(lastChar))
	{
		int ind = 0;
		do {
			curtok->identStr[ind] = lastChar;
			ind++;
			if (ind >= curtok->strLen)
			{
				curtok->strLen += 32;
				if (realloc(curtok->identStr, curtok->strLen))
					exit(-1);
			}
			lastChar = getchar();
		} while (isalnum(lastChar));
		if (strncmp("def", curtok->identStr, 3) == 0)
			tok_type = tok_def;
		else if (strncmp("extern", curtok->identStr, 6) == 0)
			tok_type = tok_extern;
		else
			tok_type = tok_ident;
	}
	else if (isdigit(lastChar) || lastChar == '.')
	{
		tok_type = tok_number;
		char numberStr[128];
		memset(numberStr, '\0', 128);
		int ind = 0;
		do {
			numberStr[ind] = lastChar;
			ind ++;
			lastChar = getchar();
		} while (isdigit(lastChar) || lastChar == '.');
		curtok->numVal = strtod(numberStr, NULL);
	}
	else if (lastChar == '#')
	{
		do {
			lastChar = getchar();
		} while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
		if (lastChar != EOF)
			tok_type = gettok();
	}
	else if (lastChar == EOF)
	{
		tok_type = tok_eof;
	}
	else
	{
		int thisChar = lastChar;
		lastChar = getchar();
		tok_type = thisChar;
	}
	curtok->tok_type = tok_type;
	return tok_type;
}
