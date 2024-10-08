#ifndef ZZ_AST_H
#define ZZ_AST_H

#include <rpr.h>

#include "ast/zz_binary_operators.h"
#include "ast/zz_unary_operators.h"

enum zz_ast_node_type {
	ZZ_AST_NODE_TYPE_NUMBER,
	ZZ_AST_NODE_TYPE_UNARY_OPERATOR,
	ZZ_AST_NODE_TYPE_BINARY_OPERATOR,
	ZZ_AST_NODE_TYPE_FUNCTION
};

struct zz_ast_node {
	enum zz_ast_node_type type;
	union {
		struct {
			rt_n value;
		} number;
		struct {
			enum zz_unary_operator unary_operator;
			struct zz_ast_node *operand;
		} unary_operator;
		struct {
			enum zz_binary_operator binary_operator;
			struct zz_ast_node *left;
			struct zz_ast_node *right;
		} binary_operator;
		struct {
			rt_char *name;
			rt_un name_size;
			struct zz_ast_node *body;
		} function;
	} u;
};

#endif /* ZZ_AST_H */
