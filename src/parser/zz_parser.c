#include "parser/zz_parser.h"

#include "parser/zz_function_parser.h"

rt_s zz_parser_parse(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **root)
{
	struct zz_token *current_token = &lexer->current_token;
	rt_s ret = RT_FAILED;

	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto end;

	if (current_token->type == ZZ_TOKEN_TYPE_END_OF_FILE) {
		/* Empty file. */
		*root = RT_NULL;
	} else {
		if (RT_UNLIKELY(!zz_function_parser_parse(lexer, ast_nodes_list, root)))
			goto end;

		/* TODO: We assume that there is a single expression for now. */
		if (RT_UNLIKELY(current_token->type != ZZ_TOKEN_TYPE_END_OF_FILE))
			goto end;
	}

	ret = RT_OK;
end:
	return ret;
}
