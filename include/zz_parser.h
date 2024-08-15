#ifndef ZZ_PARSER_H
#define ZZ_PARSER_H

#include <rpr.h>

#include "zz_lexer.h"
#include "zz_ast.h"

rt_s zz_parser_parse(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **root);

#endif /* ZZ_PARSER_H */
