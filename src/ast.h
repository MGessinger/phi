#ifndef AST_H_
#define AST_H_

#include "stack.h"

typedef enum IdentifierFlags
{
	id_any,
	id_var,
	id_func,
	id_new
} IdFlag;

typedef enum Expressions
{
	expr_literal,
	expr_binop,
	expr_ident,
	expr_proto,
	expr_func,
	expr_comm,
	expr_conditional,
	expr_loop
} ExprType;

enum Literals
{
	lit_number,
	lit_bool
};

/* General Expression type */
typedef struct Expr {
	ExprType expr_type;
	void *expr;
} Expr;

/* Specific Expression types */
typedef struct LiteralExprAST {
	double val;
	int type;
} LiteralExpr;

typedef struct BinaryExprAST {
	int op;
	void *LHS, *RHS;
} BinaryExpr;

typedef struct IdentExprAST {
	char *name;
	IdFlag flag;
} IdentExpr;

typedef struct CommandExprAST {
	Expr *head;
	Expr *tail;
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

typedef struct CondExprAST {
	Expr *Cond;
	Expr *True;
	Expr *False;
} CondExpr;

typedef struct LoopExprAST {
	Expr *Cond;
	Expr *Body;
	Expr *Else;
} LoopExpr;

Expr* newLiteralExpr (double val, int type);
Expr* newBinaryExpr (int binop, Expr *LHS, Expr *RHS);
Expr* newIdentExpr (char *name, IdFlag flag);
Expr* newProtoExpr (char *name, stack *in, stack *out);
Expr* newFunctionExpr (Expr *proto, Expr *body);
Expr* newCommandExpr (Expr *e1, Expr *e2);
Expr* newCondExpr (Expr *Cond, Expr *True, Expr *False);
Expr* newLoopExpr (Expr *Cond, Expr *body, Expr *Else);

void* logError (const char *msg, int code);
void clearExpr (Expr *e);

#endif /* AST_H_ */
