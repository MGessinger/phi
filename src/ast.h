#ifndef AST_H_
#define AST_H_

enum Expressions
{
	expr_number,
	expr_binop,
	expr_proto,
	expr_func
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

typedef struct ProtoExprAST {
	int inArgs;
	int outArgs;
	char *name;
} ProtoExpr;

typedef struct FuncExprAST {
	Expr *proto;
	Expr *body;
} FunctionExpr;

Expr* newBinaryExpr (int binop, Expr *LHS, Expr *RHS);
Expr* newNumberExpr (double val);
Expr* newProtoExpr (char *name, int in, int out);
Expr* newFunctionExpr (Expr *proto, Expr *body);

void clearExpr (Expr *e);

#endif /* AST_H_ */
