#include "parser/zz_function_parser.h"

#include "parser/zz_expression_parser.h"

rt_s zz_function_parser_parse(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
{
	struct zz_token *current_token = &lexer->current_token;
	struct zz_ast_node *ast_node;
	rt_s ret;

	if (current_token->type != ZZ_TOKEN_TYPE_FUNCTION) {
		/* TODO: Better error handling. */
		goto error;
	}

	/* Consume the fn keyword. */
	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	if (current_token->type != ZZ_TOKEN_TYPE_IDENTIFIER) {
		/* TODO: Better error handling. */
		goto error;
	}
	
	if (RT_UNLIKELY(!rt_list_new_item(ast_nodes_list, (void**)&ast_node)))
		goto error;
	
	ast_node->type = ZZ_AST_NODE_TYPE_FUNCTION;
	ast_node->u.function.name = current_token->str;
	ast_node->u.function.name_size = current_token->str_size;

	/* Consume the function name. */
	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	if (current_token->type != ZZ_TOKEN_TYPE_OPEN_PARENTHESIS) {
		/* TODO: Better error handling. */
		goto error;
	}

	/* Consume the opening parenthesis. */
	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	/* TODO: Parse arguments. */

	if (current_token->type != ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS) {
		/* TODO: Better error handling. */
		goto error;
	}

	/* Consume the closing parenthesis. */
	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	if (current_token->type != ZZ_TOKEN_TYPE_OPEN_BRACE) {
		/* TODO: Better error handling. */
		goto error;
	}

	/* Consume the opening brace. */
	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	/* Parse the body, an expression for now. */
	/* TODO: The body won't remain as just an expression for long. */
	if (RT_UNLIKELY(!zz_expression_parser_parse(lexer, ast_nodes_list, &ast_node->u.function.body)))
		goto error;

	if (current_token->type != ZZ_TOKEN_TYPE_CLOSE_BRACE) {
		/* TODO: Better error handling. */
		goto error;
	}

	/* Consume the closing brace. */
	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	*result = ast_node;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}
