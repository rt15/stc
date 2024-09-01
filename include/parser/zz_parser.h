#ifndef ZZ_PARSER_H
#define ZZ_PARSER_H

#include <rpr.h>

#include "ast/zz_ast.h"
#include "lexer/zz_lexer.h"

rt_s zz_parser_parse(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **root);

#endif /* ZZ_PARSER_H */
