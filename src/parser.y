%{
#include <stdio.h>
#include "ast.h"
#include "stack.h"
#include "codegen.h"
#include "templating.h"
%}
%union
{
	int integral;
	void *pointer;
	double numerical;
}
%token keyword_new keyword_extern keyword_from keyword_compile
%token keyword_if keyword_else keyword_while keyword_end
%token type_real type_bool type_int type_template
%token tok_new tok_var tok_func tok_arrow
%token <integral>	tok_int tok_bool tok_vec tok_array
%token <pointer>	tok_ident
%token <numerical>	tok_real

%type <integral>	TYPEARG PRIMTYPE VECTOR ARRAY TEMPCALL
%type <pointer>		TOPLEVEL QUEUE MINIMAL COMMAND IFBLOCK LOOPEXP
%type <pointer>		DECLARATION DEFINITION TYPESIG
%type <pointer>		EXPRESSION BINARYOP PRIMARY IDENTIFY MALFORMED
%type <pointer>		PARENEX SUBSCRIPT TEMPLATE

%right '='

%nonassoc '<' '>'

%left '+' '-'
%left '%'
%left '*' '/'
%{
	extern char *templateVar;
	extern int yylex();
	static int yyerror();
	static int needsName;
	const char *filename = "";
#define ERROR(a,b,c) { fprintf(stderr, "%s:%i:%i: ", filename, c.first_line, c.first_column); \
	logError(a, b); YYERROR; }
%}
%%
INPUT :
      | INPUT				{ templateVar = NULL; }
	TOPLEVEL			{ codegen($3, 1);
					  if (templateVar != NULL)
					  {
						free(templateVar);
						free($3);
					  }
					  else
						clearExpr($3); }
      ;

TOPLEVEL : error			{ $$ = NULL; }
	 | keyword_extern		{ needsName = 0; }
		DECLARATION		{ $$ = $3; }
	 | keyword_extern		{ needsName = 0; }
		TEMPLATE		{ templateVar = $3; }
		DECLARATION		{ $$ = $5; }
	 | keyword_new			{ needsName = 1; }
		DEFINITION		{ $$ = $3; }
	 | keyword_new			{ needsName = 1; }
		TEMPLATE		{ templateVar = $3; }
		DEFINITION		{ $$ = $5; }
	 | keyword_new			{ needsName = 1; }
		'!' TEMPCALL DEFINITION	{ compileTemplatePredefined($5, $4); $$ = NULL; clearExpr($5); }
	 | keyword_compile COMPILE	{ $$ = NULL; }
	 ;

COMPILE :
	| COMPILE tok_ident ':' TEMPCALL { tryGetTemplate($2, $4); }

/*===========================================*\
|* Anything related to Statements comes here *|
\*===========================================*/

QUEUE	: MINIMAL
	| QUEUE ';' MINIMAL		{ $$ = newBinaryExpr(';', $1, $3); }
	| QUEUE ';'			{ ERROR("Expected Command after ':'.", 0x1001, @2); }
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
	| IDENTIFY SUBSCRIPT		{ $$ = newAccessExpr($1, $2); }
	| IDENTIFY
	| PARENEX
	;

IDENTIFY : tok_ident			{ $$ = newIdentExpr($1, id_any, 1); }
	 | tok_ident tok_new		{ $$ = newIdentExpr($1, id_new, 1); }
	 | tok_ident tok_vec		{ $$ = newIdentExpr($1, id_vec, $2); }
	 | tok_ident tok_array		{ $$ = newIdentExpr($1, id_array, $2); }
	 | tok_ident tok_func		{ $$ = newIdentExpr($1, id_func, 1); }
	 | tok_ident tok_var		{ $$ = newIdentExpr($1, id_var, 1); }
	 | tok_ident ':' TEMPCALL	{ $$ = newTemplateExpr($1, $3); }
	 ;

OP : '+' | '-' | '*' | '/' | '>' | '<' | '=' | '%' ;

MALFORMED : EXPRESSION OP error		{ clearExpr($1); ERROR("Invalid right-hand Operand for binary operator.", 0x1101, @3); }
	  ;

/*=================================================*\
|* Anything related to Functions comes below here: *|
\*=================================================*/

DEFINITION : DECLARATION QUEUE		{ $$ = newFunctionExpr($1, $2, NULL); }
	   | DECLARATION COMMAND keyword_from QUEUE { $$ = newFunctionExpr($1, $4, $2); }
	   | DECLARATION COMMAND keyword_from error { ERROR("Expected Function Body after keyword \"from\".", 0x1702, @4); }
	   | DECLARATION error		{ ERROR("Expected Function Body after new Declaration.", 0x1701, @2); }
	   ;

DECLARATION : TYPESIG tok_arrow tok_ident { needsName = 0; }
		tok_arrow TYPESIG	{ $$ = newProtoExpr($3, $1, $6, (templateVar!=NULL)); }
	    | tok_ident			{ needsName = 0; }
		tok_arrow TYPESIG	{ $$ = newProtoExpr($1, NULL, $4, (templateVar!=NULL)); }
	    | TYPESIG tok_arrow error	{ clearStack((stack**)&($1), free); ERROR("Expected a Function Name in Prototype.", 0x1602, @3); }
	    | TYPESIG tok_arrow tok_ident error { clearStack((stack**)&($1), free); ERROR("A function must have at least one return type! Are you missing a \"->\"?", 0x1612, @2); }
	    | tok_ident error		{ free($1); ERROR("A function must have at least one return type! Are you missing a \"->\"?", 0x1611, @2); }
	    | tok_arrow			{ ERROR("Found stray \"->\" in Function Prototype. Are you missing Input Arguments?", 0x1601, @1); }
	    ;

TYPESIG : TYPEARG			{ if (needsName)
						ERROR("All function parameters must be named in the form \"Type:Name\"!", 0x1501, @1);
					  $$ = push(NULL, $1, NULL); }
	| TYPESIG TYPEARG		{ if (needsName)
						ERROR("All function parameters must be named in the form \"Type:Name\"!", 0x1501, @2);
					  $$ = push(NULL, $2, $1); }
	| TYPEARG ':' tok_ident		{ $$ = push($3, $1, NULL); }
	| TYPESIG TYPEARG ':' tok_ident	{ $$ = push($4, $2, $1); }
	;

TYPEARG : PRIMTYPE
	| PRIMTYPE VECTOR		{ $$ = $1 | $2; }
	| PRIMTYPE ARRAY		{ $$ = $1 | $2; }
	;

PRIMTYPE : type_real			{ $$ = type_real; }
	 | type_int			{ $$ = type_int; }
	 | type_bool			{ $$ = type_bool; }
	 | type_template		{ $$ = type_template; }
	 ;

/*===================================================*\
|* Anything related to parentheses comes below here: *|
\*===================================================*/

PARENEX : '(' QUEUE ')'			{ $$ = $2; }
	| '(' QUEUE error		{ clearExpr($2); ERROR("Expected ')' while parsing Command.", 0x1112, @3); }
	| '(' error			{ ERROR("Expected Command after opening '('.", 0x1111, @2); }
	;

SUBSCRIPT : '[' EXPRESSION ']'		{ $$ = $2; }
	  | '[' EXPRESSION error	{ clearExpr($2); ERROR("Expected closing ']' in subscript.", 0x1122, @3); }
	  | '[' error			{ ERROR("Expected Expression in subscript.", 0x1121, @2); }
	  ;

ARRAY : '[' tok_int ']'			{ $$ = $2 << 16; }
      | '[' tok_int error		{ ERROR("Expected closing ']' in Array declaration.", 0x1132, @3); }
      | '[' error			{ ERROR("Expected Integer in Array declaration.", 0x1131, @2); }
      ;

VECTOR : '<' tok_int '>'		{ $$ = $2 << 12; }
       | '<' tok_int error		{ ERROR("Expected closing '>' in vector declaration.", 0x1142, @3); }
       | '<' error			{ ERROR("Expected Integer in Vector declaration.", 0x1141, @2); }
       ;

TEMPLATE : '<' tok_ident '>'		{ $$ = $2; }
	 | '<' tok_ident error		{ free($2); ERROR("Expected closing '>' in template.", 0x1152, @3); }
	 | '<' error			{ ERROR("Expected template variable name after opening '<'.", 0x1151, @2); }
	 ;

TEMPCALL : '<' TYPEARG '>'		{ $$ = $2; }
	 | '<' TYPEARG error		{ ERROR("Expected closing '>' in Template Call.", 0x1162, @3); }
	 | '<' error			{ ERROR("Expected template variable after opening '<'.", 0x1161, @2); }
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
		case 0x3000:
			errtype = "Templating";
			break;
		default:
			errtype = "Unknown";
			break;
	}
	fprintf(stderr, "%s Error %x: %s\n", errtype, errcode, errstr);
	return NULL;
}
