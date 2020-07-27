#ifndef LEXER_H_
#define LEXER_H_

typedef struct token {
	int tok_type;
	double numVal;
	char *identStr;
} token;

enum Token
{
	tok_eof = -1,

	tok_def = -2,
	tok_arrow = -3,
	tok_extern = -4,

	tok_ident = -5,
	tok_number = -6,
	tok_typename = -7
};

token* makeToken ();
void clearToken (token *tk);
int gettok ();

extern token *curtok;

#endif /* LEXER_H_ */
