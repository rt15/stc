#ifndef ZZ_LEXER_H
#define ZZ_LEXER_H

#include <rpr.h>

enum zz_token_type {
	ZZ_TOKEN_TYPE_END_OF_FILE,
	ZZ_TOKEN_TYPE_IDENTIFIER,
	ZZ_TOKEN_TYPE_NUMBER,
	ZZ_TOKEN_TYPE_PLUS,
	ZZ_TOKEN_TYPE_MINUS,
	ZZ_TOKEN_TYPE_ASTERISK,
	ZZ_TOKEN_TYPE_SLASH,
	ZZ_TOKEN_TYPE_PERCENT,
	ZZ_TOKEN_TYPE_OPEN_PARENTHESIS,
	ZZ_TOKEN_TYPE_CLOSE_PARENTHESIS
};

struct zz_token {
	enum zz_token_type type;
	rt_char *str;
	rt_un str_size;
};

struct zz_lexer {
	rt_char *input;
	struct zz_token current_token;
};

rt_s zz_lexer_read_next_token(struct zz_lexer *lexer);

#endif /* ZZ_LEXER_H */
