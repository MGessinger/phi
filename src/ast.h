#ifndef AST_H_
#define AST_H_

enum Expressions
{
	expr_number,
	expr_binop,
	expr_val,
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

typedef struct VarExprAST {
	char *name;
} VarExpr;

typedef struct BinaryExprAST {
	int op;
	void *LHS, *RHS;
} BinaryExpr;

typedef struct CallExprAST {
	char *callee;
	void **args;
} CallExpr;

Expr *newBinaryExpr (int binop, Expr *LHS, Expr *RHS);
Expr *newNumberExpr (double val);

void clearExpr (Expr *e);

#endif /* AST_H_ */
