#ifndef ZZ_EXPRESSION_PARSER_H
#define ZZ_EXPRESSION_PARSER_H

#include <rpr.h>

#include "ast/zz_ast.h"
#include "lexer/zz_lexer.h"

rt_s zz_expression_parser_parse(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result);

#endif /* ZZ_EXPRESSION_PARSER_H */
