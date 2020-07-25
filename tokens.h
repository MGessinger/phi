#ifndef TOKENS_H_
#define TOKENS_H_

#include <stdlib.h>

typedef struct token {
	int tok_type;
	char *identStr;
	int strLen;
	int numVal;
} token;

enum Token
{
	tok_eof = -1,

	tok_def = -2,
	tok_extern = -3,

	tok_ident = -4,
	tok_number = -5,
};

token *makeToken (int len);
void clearToken (token *tk);
#endif /* TOKENS_H_ */
