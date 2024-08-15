#include "zz_lexer.h"

/**
 * Read something that starts with a letter or an underscore and is composed of letters, digits, and underscores.
 */
static rt_s zz_lexer_read_alpha(rt_char *input, struct zz_token *token)
{
	rt_char *in_identifier = input;

	token->type = ZZ_TOKEN_TYPE_IDENTIFIER;
	token->str = input;

	while (RT_CHAR_IS_ALPHANUM(*in_identifier) || *in_identifier == _R('_'))
		in_identifier++;
	
	token->str_size = in_identifier - input;

	return RT_OK;
}

static rt_s zz_lexer_read_num(rt_char *input, struct zz_token *token)
{
	rt_char *in_identifier = input;

	token->type = ZZ_TOKEN_TYPE_NUMBER;
	token->str = input;

	while (RT_CHAR_IS_NUM(*in_identifier))
		in_identifier++;
	
	token->str_size = in_identifier - input;

	return RT_OK;
}

rt_s zz_lexer_read_next_token(struct zz_lexer *lexer)
{
	rt_char *input = lexer->input;
	struct zz_token *current_token = &lexer->current_token;
	rt_char character;
	rt_s ret;

	while (*input && RT_CHAR_IS_BLANK(*input))
		input++;

	character = *input;
	if (RT_CHAR_IS_ALPHA(character) || character == _R('_')) {
		if (RT_UNLIKELY(!zz_lexer_read_alpha(input, current_token)))
			goto error;
	} else if (RT_CHAR_IS_NUM(character)) {
		if (RT_UNLIKELY(!zz_lexer_read_num(input, &lexer->current_token)))
			goto error;
	} else if (character == _R('+')) {
		current_token->type = ZZ_TOKEN_TYPE_PLUS;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (character == _R('-')) {
		current_token->type = ZZ_TOKEN_TYPE_MINUS;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (character == _R('*')) {
		current_token->type = ZZ_TOKEN_TYPE_ASTERISK;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (character == _R('/')) {
		current_token->type = ZZ_TOKEN_TYPE_SLASH;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (character == _R('%')) {
		current_token->type = ZZ_TOKEN_TYPE_PERCENT;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (character == _R('(')) {
		current_token->type = ZZ_TOKEN_TYPE_OPEN_PARENTHESIS;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (character == _R(')')) {
		current_token->type = ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS;
		current_token->str = input;
		current_token->str_size = 1;
	} else if (!character) {
		current_token->type = ZZ_TOKEN_TYPE_END_OF_FILE;
		current_token->str = RT_NULL;
		current_token->str_size = 0;
	} else {
		/* TODO. */
	}
	lexer->input = input + current_token->str_size;

	if (current_token->type != ZZ_TOKEN_TYPE_END_OF_FILE) {
		if (RT_UNLIKELY(!rt_console_write_str_with_size(current_token->str, current_token->str_size)))
			goto error;
		if (RT_UNLIKELY(!rt_console_write_str_with_size(_R("\n"), 1)))
			goto error;
	} else {
		if (RT_UNLIKELY(!rt_console_write_str_with_size(_R("EOF\n"), 4)))
			goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}
