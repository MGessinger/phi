%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "stack.h"
#include "codegen.h"
%}
%union
{
	double numerical;
	char *string;
	void *pointer;
}
%token <numerical> tok_number tok_arr
%token <string> tok_ident
%token keyword_new keyword_extern keyword_just
%token <numerical> type_number type_string type_bool

%type <numerical>	TYPEARG
%type <string>		DECBODY
%type <pointer>		DECLARATION DEFINITION TYPESIG
%type <pointer>		TOPLEVEL COMMAND EXPRESSION PRIMARY BINARYOP MALFORMED

%right '='

%nonassoc '<' '>'

%left '+' '-'
%left '*' '/'
%{
	int yylex();
	int yyerror();
	void printError (const char *errmsg, int errcode);
	int needsName;
#define ERROR(a,b) { logError(a, b); YYERROR; }
%}
%%
INPUT :
      | INPUT TOPLEVEL			{ codegen($2); clearExpr($2); }
      ;

TOPLEVEL : TOPLEVEL ';'			{ $$ = $1; }
	 | error			{ $$ = NULL; }
	 | keyword_extern		{ needsName = 0; }
		DECLARATION		{ printf("Parsed external declaration\n");	$$ = $3; }
	 | keyword_new			{ needsName = 1; }
		DEFINITION		{ printf("Parsed function definition\n");	$$ = $3; }
	 | keyword_just COMMAND		{ printf("Parsed top-level command\n");
					  char *name = malloc(sizeof(char));
					  name[0] = '\0';
					  Expr *anon = newProtoExpr (name, NULL, NULL);
					  $$ = newFunctionExpr(anon, $2); }
	 ;

DECBODY : tok_ident			{ needsName = 0; }
		tok_arr			{ $$ = $1; }
	| tok_ident			{ free($1); ERROR("A function must have at least one return type! Are you missing a \"->\"?", 0x1400); }
	;

DECLARATION : DECBODY TYPESIG		{ stack *outArgs = $2;
					  if (outArgs == NULL)
						ERROR("A Function must have at least one return type.", 0x1202)
					  else
						$$ = newProtoExpr($1, NULL, outArgs); }
	    | TYPESIG tok_arr DECBODY TYPESIG	{ stack *inArgs = $1;
					  stack *outArgs = $4;
					  if (inArgs == NULL)
						  ERROR("Found stray \"->\" in Function Prototype.", 0x1201)
					  else if (outArgs == NULL)
					  {
						  clearStack(inArgs, free);
						  ERROR("A Function must have at least one return type.", 0x1202)
					  }
					  else
						  $$ = newProtoExpr($3, inArgs, outArgs); }
	    | TYPESIG error		{ clearStack($1, free); ERROR("Expected a Function Name in Prototype.", 0x1203); }
	    ;

TYPESIG :				{ $$ = NULL; }
	| TYPESIG TYPEARG ':' tok_ident	{ $$ = push($4, $2, $1); }
	| TYPESIG TYPEARG		{ if(needsName)
						ERROR("All function parameters must be named in the form \"Type:Name\"!", 0x1300);
					  $$ = push(NULL, $2, $1); }
	;

DEFINITION : DECLARATION COMMAND	{ $$ = newFunctionExpr($1, $2); }
	   ;

BINARYOP : EXPRESSION '+' EXPRESSION	{ $$ = newBinaryExpr('+', $1, $3); }
	 | EXPRESSION '-' EXPRESSION	{ $$ = newBinaryExpr('-', $1, $3); }
	 | EXPRESSION '*' EXPRESSION	{ $$ = newBinaryExpr('*', $1, $3); }
	 | EXPRESSION '/' EXPRESSION	{ $$ = newBinaryExpr('/', $1, $3); }
	 | EXPRESSION '>' EXPRESSION	{ $$ = newBinaryExpr('>', $1, $3); }
	 | EXPRESSION '<' EXPRESSION	{ $$ = newBinaryExpr('<', $1, $3); }
	 ;

OP : '+' | '-' | '*' | '/' | '>' | '<' ;

MALFORMED : EXPRESSION OP error 	{ clearExpr($1); ERROR("Invalid right-hand Operand for binary operator.", 0x1500); }
	  ;

EXPRESSION : BINARYOP
	   | PRIMARY
	   | MALFORMED
	   ;

COMMAND : EXPRESSION			{ Expr *new = $1;
					  if (new->expr_type == expr_named)
						$$ = newCallExpr($1, NULL);
					  else
						$$ = $1; }
	| COMMAND EXPRESSION		{ Expr *new = $2;
					  if (new->expr_type == expr_named)
						$$ = newCallExpr($2, $1);
					  else
						$$ = newCommandExpr($1, $2); }
	;

PRIMARY : tok_number			{ $$ = newNumberExpr($1); }
	| tok_ident			{ LLVMValueRef fn = tryGetNamedFunc($1);
					  if (fn != NULL)
					  {
						free($1);
						$$ = newNamedExpr(fn);
					  }
					  else
						$$ = newIdentExpr($1); }
	| '(' COMMAND ')'		{ $$ = $2; }
	| '(' COMMAND error		{ clearExpr($2); ERROR("Expected ')' while parsing Expression.", 0x1100); }
	;

TYPEARG : type_number
	| type_string
	| type_bool
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
