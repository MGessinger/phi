%{
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "stack.h"
#include "codegen.h"
%}
%union
{
	double numerical;
	int integral;
	char *string;
	void *pointer;
}
%token keyword_new keyword_extern
%token keyword_if keyword_else keyword_while keyword_end
%token <string>		tok_ident tok_new tok_var tok_func
%token <numerical>	tok_real type_real tok_arr
%token <integral>	type_int tok_int type_bool tok_bool

%type <integral>	TYPEARG PRIMTYPE
%type <string>		DECBODY
%type <pointer>		TOPLEVEL MINIMAL COMMAND QUEUE
%type <pointer>		DECLARATION DEFINITION TYPESIG
%type <pointer>		EXPRESSION PRIMARY BINARYOP MALFORMED IFBLOCK LOOPEXP IDENTIFY

%right '='

%nonassoc '<' '>'

%left '+' '-'
%left '%'
%left '*' '/'
%{
	int yylex();
	int yyerror();
	int needsName;
#define ERROR(a,b) { logError(a, b); YYERROR; }
%}
%%
INPUT :
      | INPUT TOPLEVEL			{ codegen($2, 1); clearExpr($2); }
      ;

TOPLEVEL : error			{ $$ = NULL; }
	 | keyword_extern		{ needsName = 0; }
		DECLARATION		{ $$ = $3; }
	 | keyword_new			{ needsName = 1; }
		DEFINITION		{ $$ = $3; }
	 ;

/*===========================================*\
|* Anything related to Statements comes here *|
\*===========================================*/

QUEUE	: MINIMAL
	| QUEUE ':' MINIMAL		{ $$ = newBinaryExpr(':', $1, $3); }
	;

MINIMAL : COMMAND
	| IFBLOCK
	| LOOPEXP
	;

COMMAND : EXPRESSION
	| COMMAND EXPRESSION		{ $$ = newCommandExpr($1, $2); }
	;

IFBLOCK : keyword_if EXPRESSION MINIMAL keyword_else MINIMAL	{ $$ = newCondExpr($2, $3, $5); }
	| keyword_if EXPRESSION MINIMAL keyword_end		{ $$ = newCondExpr($2, $3, NULL); }
	| keyword_if EXPRESSION MINIMAL error			{ clearExpr($2); clearExpr($3); ERROR("Expected \"end\" or \"else\" after Conditional Expression.", 0x1303); }
	| keyword_if EXPRESSION error				{ clearExpr($2); ERROR("Expected Command in Conditional statement.", 0x1302); }
	| keyword_if error					{ ERROR("Expected Conditional Expression after \"if\".", 0x1301); }
	;

LOOPEXP : keyword_while EXPRESSION MINIMAL keyword_else MINIMAL	{ $$ = newLoopExpr($2, $3, $5); }
	| keyword_while EXPRESSION MINIMAL keyword_end		{ $$ = newLoopExpr($2, $3, NULL); }
	| keyword_while EXPRESSION MINIMAL error		{ clearExpr($2); clearExpr($3); ERROR("Expected \"end\" or \"else\" after Loop Expression.", 0x1403); }
	| keyword_while EXPRESSION error			{ clearExpr($2); ERROR("Expected Command in Loop Body.", 0x1402); }
	| keyword_while error					{ ERROR("Expected Conditional Expression in Loop Head.", 0x1401); }
	;

EXPRESSION : BINARYOP
	   | PRIMARY
	   | MALFORMED
	   ;

BINARYOP : EXPRESSION '+' EXPRESSION	{ $$ = newBinaryExpr('+', $1, $3); }
	 | EXPRESSION '-' EXPRESSION	{ $$ = newBinaryExpr('-', $1, $3); }
	 | EXPRESSION '*' EXPRESSION	{ $$ = newBinaryExpr('*', $1, $3); }
	 | EXPRESSION '/' EXPRESSION	{ $$ = newBinaryExpr('/', $1, $3); }
	 | EXPRESSION '<' EXPRESSION	{ $$ = newBinaryExpr('<', $1, $3); }
	 | EXPRESSION '>' EXPRESSION	{ $$ = newBinaryExpr('<', $3, $1); } /* Switched the Arguments! */
	 | EXPRESSION '=' EXPRESSION	{ $$ = newBinaryExpr('=', $1, $3); }
	 | EXPRESSION '%' EXPRESSION	{ $$ = newBinaryExpr('%', $1, $3); }
	 ;

PRIMARY : tok_bool			{ $$ = newLiteralExpr($1, lit_bool); }
	| tok_real			{ $$ = newLiteralExpr($1, lit_real); }
	| tok_int			{ $$ = newLiteralExpr($1, lit_int); }
	| '(' QUEUE ')'			{ $$ = $2; }
	| '(' QUEUE error		{ clearExpr($2); ERROR("Expected ')' while parsing Command.", 0x1101); }
	| IDENTIFY			{ $$ = $1; }
	| IDENTIFY '[' EXPRESSION ']'	{ $$ = newAccessExpr($1, $3); }
	| IDENTIFY '[' EXPRESSION error	{ clearExpr($1); clearExpr($3); ERROR("Expected closing ']' in array index.", 0x1102); }
	;

IDENTIFY : tok_ident			{ $$ = newIdentExpr($1, id_any); }
	 | tok_new			{ $$ = newIdentExpr($1, id_new); }
	 | tok_func			{ $$ = newIdentExpr($1, id_func); }
	 | tok_var			{ $$ = newIdentExpr($1, id_var); }
	 ;

OP : '+' | '-' | '*' | '/' | '>' | '<' | '=' | '%' ;

MALFORMED : EXPRESSION OP error		{ clearExpr($1); ERROR("Invalid right-hand Operand for binary operator.", 0x1201); }
	  ;

/*=================================================*\
|* Anything related to Functions comes below here: *|
\*=================================================*/

DEFINITION : DECLARATION QUEUE		{ $$ = newFunctionExpr($1, $2); }
	   ;

DECLARATION : DECBODY TYPESIG		{ stack *outArgs = $2;
					  if (outArgs == NULL)
						ERROR("A Function must have at least one return type.", 0x1803)
					  else
						$$ = newProtoExpr($1, NULL, outArgs); }
	    | TYPESIG tok_arr DECBODY TYPESIG	{ stack *inArgs = $1;
					  stack *outArgs = $4;
					  if (inArgs == NULL)
						  ERROR("Found stray \"->\" in Function Prototype.", 0x1801)
					  else if (outArgs == NULL)
					  {
						  clearStack(&inArgs, free);
						  ERROR("A Function must have at least one return type.", 0x1803)
					  }
					  else
						  $$ = newProtoExpr($3, inArgs, outArgs); }
	    | TYPESIG tok_arr error	{ clearStack((stack**)&($1), free); ERROR("Expected a Function Name in Prototype.", 0x1702); }
	    ;

DECBODY : tok_ident tok_arr		{ needsName = 0; $$ = $1; }
	| tok_ident			{ free($1); ERROR("A function must have at least one return type! Are you missing a \"->\"?", 0x1701); }
	;

TYPESIG :				{ $$ = NULL; }
	| TYPESIG TYPEARG ':' tok_ident	{ $$ = push($4, $2, $1); }
	| TYPESIG TYPEARG		{ if (needsName)
						ERROR("All function parameters must be named in the form \"Type:Name\"!", 0x1601);
					  $$ = push(NULL, $2, $1); }
	;

TYPEARG : PRIMTYPE
	| PRIMTYPE '<' tok_int '>'	{ $$ = ($1 | ((int)$3 << 16)); }
	| PRIMTYPE '<' tok_int error	{ ERROR("Expected closing '>' in vector declaration.", 0x1502); }
	| PRIMTYPE '<' error		{ ERROR("Expected Integer in Vector declaration.", 0x1501); }
	;

PRIMTYPE : type_real			{ $$ = type_real; }
	 | type_int			{ $$ = type_int; }
	 | type_bool			{ $$ = type_bool; }
	 ;
%%
int yyerror()
{
	return 1;
}

void* logError(const char *errstr, int errcode)
{
	const char *errtype;
	switch (errcode & 0xF000)
	{
		case 0x0000:
			errtype = "Internal";
			break;
		case 0x1000:
			errtype = "Syntax";
			break;
		case 0x2000:
			errtype = "Compilation";
			break;
		default:
			errtype = "Unknown";
			break;
	}
	fprintf(stderr, "%s Error %x: %s\n", errtype, errcode, errstr);
	return NULL;
}
