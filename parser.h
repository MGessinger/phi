#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

typedef struct NumExprAST {
	double val;
} NumExpr;

typedef struct VarExprAST {
	char *name;
} VarExpr;

typedef struct BinaryExprAST {
	char op;
	void *LHS, *RHS;
} BinaryExpr;

typedef struct CallExprAST {
	char *callee;
	void **args;
} CallExpr;

void* parseNumberExpr (token *curtok);

#endif /* PARSER_H_ */
