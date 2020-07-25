#include "tokens.h"

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
