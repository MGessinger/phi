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

int gettok(token* tk)
{
	static char lastChar;
	do {
		lastChar = getchar();
	} while (isspace(lastChar));
	if (isalpha(lastChar))
	{
		int ind = 0;
		do {
			tk->identStr[ind] = lastChar;
			ind++;
			if (ind >= tk->strLen)
			{
				tk->strLen += 32;
				if (realloc(tk->identStr, tk->strLen))
					exit(-1);
			}
			lastChar = getchar();
		} while (isalnum(lastChar));
		if (strncmp("def", tk->identStr, 3) == 0)
			return tok_def;
		else if (strncmp("extern", tk->identStr, 6) == 0)
			return tok_extern;
		else
			return tok_ident;
	}
	else if (isdigit(lastChar) || lastChar == '.')
	{
		char numberStr[128];
		memset(numberStr, '\0', 128);
		int ind = 0;
		do {
			numberStr[ind] = lastChar;
			ind ++;
			lastChar = getchar();
		} while (isdigit(lastChar) || lastChar == '.');
		tk->numVal = strtod(numberStr, NULL);
		return tok_number;
	}
	else if (lastChar == '#')
	{
		do {
			lastChar = getchar();
		} while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
		if (lastChar != EOF)
			return gettok(tk);
	}
	else if (lastChar == EOF)
	{
		return tok_eof;
	}
	else
	{
		int thisChar = lastChar;
		lastChar = getchar();
		return thisChar;
	}
	return 0;
}
