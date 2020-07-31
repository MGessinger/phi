#ifndef AST_H_
#define AST_H_

#include "stack.h"

enum Expressions
{
	expr_number,
	expr_binop,
	expr_ident,
	expr_proto,
	expr_func,
	expr_comm,
	expr_named,
	expr_call
};

/* General Expression type */
typedef struct Expr {
	int expr_type;
	void *expr;
} Expr;

/* Specific Expression types */
typedef struct NumExprAST {
	double val;
} NumExpr;

typedef struct BinaryExprAST {
	int op;
	void *LHS, *RHS;
} BinaryExpr;

typedef struct IdentExprAST {
	char *name;
} IdentExpr;

typedef struct CommandExprAST {
	Expr *es[2];
} CommandExpr;

typedef struct ProtoExprAST {
	stack *inArgs;
	stack *outArgs;
	char *name;
} ProtoExpr;

typedef struct FuncExprAST {
	Expr *proto;
	Expr *body;
} FunctionExpr;

typedef struct CallExprAST {
	void *funcRef;
	Expr **args;
	unsigned numArgs;
} CallExpr;

typedef struct NamedExprAST {
	void *funcRef;
} NamedExpr;

Expr* newNumberExpr (double val);
Expr* newBinaryExpr (int binop, Expr *LHS, Expr *RHS);
Expr* newIdentExpr (char *name);
Expr* newProtoExpr (char *name, stack *in, stack *out);
Expr* newFunctionExpr (Expr *proto, Expr *body);
Expr* newCommandExpr (Expr *e1, Expr *e2);
Expr* newCallExpr (Expr *funcRef, Expr *args);
Expr* newNamedExpr (void *funcRef);

void* logError(const char *msg, int code);
void clearExpr (Expr *e);

#endif /* AST_H_ */
