#include "parser/zz_parser.h"

static const rt_un zz_parser_binary_operators_precedence[] = {
	[ZZ_BINARY_OPERATOR_ADD] = 1,
	[ZZ_BINARY_OPERATOR_SUBTRACT] = 1,
	[ZZ_BINARY_OPERATOR_MULTIPLY] = 2,
	[ZZ_BINARY_OPERATOR_DIVIDE] = 2,
	[ZZ_BINARY_OPERATOR_MODULO] = 2
};

static rt_s zz_parser_parse_expression(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result);

/**
 * Parse minus as a unary operator.
 */
static rt_s zz_parser_parse_minus(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
{
	rt_s ret;

	if (RT_UNLIKELY(!rt_list_new_item(ast_nodes_list, (void**)result)))
		goto error;

	(*result)->type = ZZ_AST_NODE_TYPE_UNARY_OPERATOR;
	(*result)->u.unary_operator.unary_operator = ZZ_UNARY_OPERATOR_NEGATE;

	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

/**
 * Parse a number.
 * 
 * <p>
 * The possible minus must have been parsed already.
 * </p>
 */
static rt_s zz_parser_parse_number(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
{
	struct zz_token *current_token = &lexer->current_token;
	rt_n value;
	rt_s ret;

	if (RT_UNLIKELY(!rt_char_convert_to_n_with_size(current_token->str, current_token->str_size, &value)))
		goto error;

	if (RT_UNLIKELY(!rt_list_new_item(ast_nodes_list, (void**)result)))
		goto error;

	(*result)->type = ZZ_AST_NODE_TYPE_NUMBER;
	(*result)->u.number.value = value;

	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_parser_parse_parenthesis(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
{
	struct zz_token *current_token = &lexer->current_token;
	rt_s ret;

	/* Consume the opening parenthesis. */
	if (!zz_lexer_read_next_token(lexer))
		return RT_FAILED;

	if (RT_UNLIKELY(!zz_parser_parse_expression(lexer, ast_nodes_list, result)))
		goto error;

	if (current_token->type != ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS) {
		/* TODO: Better error handling. */
		goto error;
	}

	/* Consume the closing parenthesis. */
	if (!zz_lexer_read_next_token(lexer))
		return RT_FAILED;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

/**
 * A binary operator is not a primary.
 */
static rt_s zz_parser_parse_primary(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
{
	struct zz_token *current_token = &lexer->current_token;
	struct zz_ast_node *ast_node;
	rt_s ret;

	switch (current_token->type) {
	case ZZ_TOKEN_TYPE_MINUS:
		/* Unary minus. */
		if (RT_UNLIKELY(!zz_parser_parse_minus(lexer, ast_nodes_list, result)))
			goto error;
		if (RT_UNLIKELY(!zz_parser_parse_primary(lexer, ast_nodes_list, &ast_node)))
			goto error;
		(*result)->u.unary_operator.operand = ast_node;
		break;
	case ZZ_TOKEN_TYPE_NUMBER:
		if (RT_UNLIKELY(!zz_parser_parse_number(lexer, ast_nodes_list, result)))
			goto error;
		break;
	case ZZ_TOKEN_TYPE_OPEN_PARENTHESIS:
		if (RT_UNLIKELY(!zz_parser_parse_parenthesis(lexer, ast_nodes_list, result)))
			goto error;
		break;
	default:
		/* TODO: Better error handling. */
		goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_parser_parse_binary_operator(struct zz_lexer *lexer, rt_un parent_operator_precedence, struct zz_ast_node *left_hand_side, void **ast_nodes_list, struct zz_ast_node **result)
{
	struct zz_token *current_token = &lexer->current_token;
	enum zz_binary_operator current_operator;
	rt_un current_operator_precedence;
	enum zz_binary_operator next_operator;
	rt_un next_operator_precedence;
	struct zz_ast_node *right_hand_side;
	struct zz_ast_node *ast_node;
	rt_s ret;

	while (RT_TRUE) {

		/* TODO: I probably want end of instruction here. */
		if (current_token->type == ZZ_TOKEN_TYPE_END_OF_FILE ||
		    current_token->type == ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS ||
		    current_token->type == ZZ_TOKEN_TYPE_CLOSE_BRACE) {
			*result = left_hand_side;
			break;
		}

		switch (current_token->type) {
		case ZZ_TOKEN_TYPE_PLUS:
			current_operator = ZZ_BINARY_OPERATOR_ADD;
			break;
		case ZZ_TOKEN_TYPE_MINUS:
			current_operator = ZZ_BINARY_OPERATOR_SUBTRACT;
			break;
		case ZZ_TOKEN_TYPE_ASTERISK:
			current_operator = ZZ_BINARY_OPERATOR_MULTIPLY;
			break;
		case ZZ_TOKEN_TYPE_SLASH:
			current_operator = ZZ_BINARY_OPERATOR_DIVIDE;
			break;
		case ZZ_TOKEN_TYPE_PERCENT:
			current_operator = ZZ_BINARY_OPERATOR_MODULO;
			break;
		case ZZ_TOKEN_TYPE_END_OF_FILE:
		case ZZ_TOKEN_TYPE_IDENTIFIER:
		case ZZ_TOKEN_TYPE_FUNCTION:
		case ZZ_TOKEN_TYPE_NUMBER:
		default:
			/* TODO: Better error handling. */
			goto error;
		}

		/* TODO: And now I probably really want a binary operator here. */
		current_operator_precedence = zz_parser_binary_operators_precedence[current_operator];
		if (current_operator_precedence < parent_operator_precedence) {
			*result = left_hand_side;
			break;
		}

		/* We switch from the operator to the first primary after it. */
		if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
			goto error;

		/* We must have a primary after a binary operator. */
		if (RT_UNLIKELY(!zz_parser_parse_primary(lexer, ast_nodes_list, &right_hand_side)))
			goto error;

		/* Now we expect either a binary operator or the end of the expression. */
		if (current_token->type != ZZ_TOKEN_TYPE_END_OF_FILE &&
		    current_token->type != ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS &&
		    current_token->type != ZZ_TOKEN_TYPE_CLOSE_BRACE) {

			switch (current_token->type) {
			case ZZ_TOKEN_TYPE_PLUS:
				next_operator = ZZ_BINARY_OPERATOR_ADD;
				break;
			case ZZ_TOKEN_TYPE_MINUS:
				next_operator = ZZ_BINARY_OPERATOR_SUBTRACT;
				break;
			case ZZ_TOKEN_TYPE_ASTERISK:
				next_operator = ZZ_BINARY_OPERATOR_MULTIPLY;
				break;
			case ZZ_TOKEN_TYPE_SLASH:
				next_operator = ZZ_BINARY_OPERATOR_DIVIDE;
				break;
			case ZZ_TOKEN_TYPE_PERCENT:
				next_operator = ZZ_BINARY_OPERATOR_MODULO;
				break;
			case ZZ_TOKEN_TYPE_END_OF_FILE:
			case ZZ_TOKEN_TYPE_IDENTIFIER:
			case ZZ_TOKEN_TYPE_FUNCTION:
			case ZZ_TOKEN_TYPE_NUMBER:
			case ZZ_TOKEN_TYPE_OPEN_PARENTHESIS:
			case ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS:
			case ZZ_TOKEN_TYPE_OPEN_BRACE:
			case ZZ_TOKEN_TYPE_CLOSE_BRACE:
			default:
				/* TODO: Better error handling. */
				goto error;
			}

			next_operator_precedence = zz_parser_binary_operators_precedence[next_operator];
			if (current_operator_precedence < next_operator_precedence) {
				if (RT_UNLIKELY(!zz_parser_parse_binary_operator(lexer, current_operator_precedence + 1, right_hand_side, ast_nodes_list, result)))
					goto error;
				right_hand_side = *result;
			}
		}

		if (RT_UNLIKELY(!rt_list_new_item(ast_nodes_list, (void**)&ast_node)))
			goto error;

		ast_node->type = ZZ_AST_NODE_TYPE_BINARY_OPERATOR;
		ast_node->u.binary_operator.binary_operator = current_operator;
		ast_node->u.binary_operator.left = left_hand_side;
		ast_node->u.binary_operator.right = right_hand_side;
		left_hand_side = ast_node;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_parser_parse_expression(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
{
	struct zz_token *current_token = &lexer->current_token;
	struct zz_ast_node *left_hand_side;
	rt_s ret;

	/* Parse the left hand side. */
	if (RT_UNLIKELY(!zz_parser_parse_primary(lexer, ast_nodes_list, &left_hand_side)))
		goto error;

	switch (current_token->type) {
	case ZZ_TOKEN_TYPE_END_OF_FILE:
	case ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS:
	case ZZ_TOKEN_TYPE_CLOSE_BRACE:
		/* It was just a primary, without binary operator and right hand side expression. */
		*result = left_hand_side;
		break;
	case ZZ_TOKEN_TYPE_PLUS:
	case ZZ_TOKEN_TYPE_MINUS:
	case ZZ_TOKEN_TYPE_ASTERISK:
	case ZZ_TOKEN_TYPE_SLASH:
	case ZZ_TOKEN_TYPE_PERCENT:
		if (RT_UNLIKELY(!zz_parser_parse_binary_operator(lexer, 0, left_hand_side, ast_nodes_list, result)))
			goto error;
		break;
	default:
		/* TODO: Handle error. */
		goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_parser_parse_function(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **result)
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
	if (RT_UNLIKELY(!zz_parser_parse_expression(lexer, ast_nodes_list, &ast_node->u.function.body)))
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

rt_s zz_parser_parse(struct zz_lexer *lexer, void **ast_nodes_list, struct zz_ast_node **root)
{
	struct zz_token *current_token = &lexer->current_token;
	rt_s ret;

	if (RT_UNLIKELY(!zz_lexer_read_next_token(lexer)))
		goto error;

	if (current_token->type == ZZ_TOKEN_TYPE_END_OF_FILE) {
		/* Empty file. */
		*root = RT_NULL;
	} else {
		if (RT_UNLIKELY(!zz_parser_parse_function(lexer, ast_nodes_list, root)))
			goto error;

		/* TODO: We assume that there is a single expression for now. */
		if (RT_UNLIKELY(current_token->type != ZZ_TOKEN_TYPE_END_OF_FILE))
			goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}
