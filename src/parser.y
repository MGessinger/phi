%{
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "stack.h"
#include "codegen.h"
%}
%union
{
	int integral;
	void *pointer;
	double numerical;
}
%token keyword_new keyword_extern keyword_from
%token keyword_if keyword_else keyword_while keyword_end
%token type_real type_bool type_int
%token tok_new tok_var tok_func tok_arrow
%token <integral>	tok_int tok_bool tok_vec tok_array
%token <pointer>	tok_ident
%token <numerical>	tok_real

%type <integral>	TYPEARG PRIMTYPE VECTORS ARRAYS
%type <pointer>		TOPLEVEL QUEUE MINIMAL COMMAND IFBLOCK LOOPEXP
%type <pointer>		DECLARATION DEFINITION TYPESIG DECBODY
%type <pointer>		EXPRESSION BINARYOP PRIMARY IDENTIFY MALFORMED

%right '='

%nonassoc '<' '>'

%left '+' '-'
%left '%'
%left '*' '/'
%{
	extern int yylex();
	static int yyerror();
	static int needsName;
	const char *filename = "";
#define ERROR(a,b,c) { fprintf(stderr, "%s:%i:%i: ", filename, c.first_line, c.first_column); \
	logError(a, b); YYERROR; }
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
	| QUEUE ':'			{ ERROR("Expected Command after ':'.", 0x1001, @2); }
	;

MINIMAL : COMMAND
	| IFBLOCK
	| LOOPEXP
	;

COMMAND : EXPRESSION
	| COMMAND EXPRESSION		{ $$ = newBinaryExpr(' ', $1, $2); }
	;

IFBLOCK : keyword_if EXPRESSION MINIMAL keyword_else MINIMAL	{ $$ = newCondExpr($2, $3, $5); }
	| keyword_if EXPRESSION MINIMAL keyword_end		{ $$ = newCondExpr($2, $3, NULL); }
	| keyword_if EXPRESSION MINIMAL error			{ clearExpr($2); clearExpr($3); ERROR("Expected \"end\" or \"else\" after Conditional Expression.", 0x1303, @4); }
	| keyword_if EXPRESSION error				{ clearExpr($2); ERROR("Expected Command in Conditional statement.", 0x1302, @3); }
	| keyword_if error					{ ERROR("Expected Conditional Expression after \"if\".", 0x1301, @2); }
	;

LOOPEXP : keyword_while EXPRESSION MINIMAL keyword_else MINIMAL	{ $$ = newLoopExpr($2, $3, $5); }
	| keyword_while EXPRESSION MINIMAL keyword_end		{ $$ = newLoopExpr($2, $3, NULL); }
	| keyword_while EXPRESSION MINIMAL error		{ clearExpr($2); clearExpr($3); ERROR("Expected \"end\" or \"else\" after Loop Expression.", 0x1403, @4); }
	| keyword_while EXPRESSION error			{ clearExpr($2); ERROR("Expected Command in Loop Body.", 0x1402, @3); }
	| keyword_while error					{ ERROR("Expected Conditional Expression in Loop Head.", 0x1401, @2); }
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
	| '(' QUEUE error		{ clearExpr($2); ERROR("Expected ')' while parsing Command.", 0x1101, @3); }
	| IDENTIFY			{ $$ = $1; }
	| IDENTIFY '[' EXPRESSION ']'	{ $$ = newAccessExpr($1, $3); }
	| IDENTIFY '[' EXPRESSION error	{ clearExpr($1); clearExpr($3); ERROR("Expected closing ']' in subscript.", 0x1102, @4); }
	;

IDENTIFY : tok_ident			{ $$ = newIdentExpr($1, id_any, 1); }
	 | tok_ident tok_new		{ $$ = newIdentExpr($1, id_new, 1); }
	 | tok_ident tok_vec		{ $$ = newIdentExpr($1, id_vec, $2); }
	 | tok_ident tok_array		{ $$ = newIdentExpr($1, id_array, $2); }
	 | tok_ident tok_func		{ $$ = newIdentExpr($1, id_func, 1); }
	 | tok_ident tok_var		{ $$ = newIdentExpr($1, id_var, 1); }
	 ;

OP : '+' | '-' | '*' | '/' | '>' | '<' | '=' | '%' ;

MALFORMED : EXPRESSION OP error		{ clearExpr($1); ERROR("Invalid right-hand Operand for binary operator.", 0x1201, @3); }
	  ;

/*=================================================*\
|* Anything related to Functions comes below here: *|
\*=================================================*/

DEFINITION : DECLARATION QUEUE		{ $$ = newFunctionExpr($1, $2, NULL); }
	   | DECLARATION COMMAND keyword_from QUEUE { $$ = newFunctionExpr($1, $4, $2); }
	   | DECLARATION COMMAND keyword_from error { ERROR("Expected Function Body after keyword \"from\".", 0x1901, @4); }
	   | DECLARATION error		{ ERROR("Expected Function Body after new Declaration.", 0x1902, @2); }
	   ;

DECLARATION : DECBODY TYPESIG		{ stack *outArgs = $2;
					  if (outArgs == NULL)
						ERROR("A Function must have at least one return type.", 0x1803, @2)
					  else
						$$ = newProtoExpr($1, NULL, outArgs); }
	    | TYPESIG tok_arrow DECBODY TYPESIG	{ stack *inArgs = $1;
					  stack *outArgs = $4;
					  if (inArgs == NULL)
						  ERROR("Found stray \"->\" in Function Prototype.", 0x1801, @1)
					  else if (outArgs == NULL)
					  {
						  clearStack(&inArgs, free);
						  ERROR("A Function must have at least one return type.", 0x1803, @4)
					  }
					  else
						  $$ = newProtoExpr($3, inArgs, outArgs); }
	    | TYPESIG tok_arrow error	{ clearStack((stack**)&($1), free); ERROR("Expected a Function Name in Prototype.", 0x1802, @3); }
	    ;

DECBODY : tok_ident tok_arrow		{ needsName = 0; $$ = $1; }
	| tok_ident			{ free($1); ERROR("A function must have at least one return type! Are you missing a \"->\"?", 0x1701, @1); }
	;

TYPESIG :				{ $$ = NULL; }
	| TYPESIG TYPEARG ':' tok_ident	{ $$ = push($4, $2, $1); }
	| TYPESIG TYPEARG		{ if (needsName)
						ERROR("All function parameters must be named in the form \"Type:Name\"!", 0x1601, @2);
					  $$ = push(NULL, $2, $1); }
	;

TYPEARG : PRIMTYPE
	| VECTORS
	| ARRAYS
	;

ARRAYS : PRIMTYPE '[' tok_int ']'	{ $$ = ($1 | ($3 << 16)); }
       | PRIMTYPE '[' tok_int error	{ ERROR("Expected closing ']' in Array declaration.", 0x1512, @4); }
       | PRIMTYPE '[' error		{ ERROR("Expected Integer in Array declaration.", 0x1511, @3); }
       ;

VECTORS : PRIMTYPE '<' tok_int '>'	{ $$ = ($1 | ($3 << 12)); }
	| PRIMTYPE '<' tok_int error	{ ERROR("Expected closing '>' in vector declaration.", 0x1522, @4); }
	| PRIMTYPE '<' error		{ ERROR("Expected Integer in Vector declaration.", 0x1521, @3); }
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
