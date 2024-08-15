#ifndef ZZ_CODE_GENERATOR_H
#define ZZ_CODE_GENERATOR_H

#include <rpr.h>

#include "zz_ast.h"

rt_s zz_code_generator_generate(struct zz_ast_node *root, rt_char *output_file_path);

#endif /* ZZ_CODE_GENERATOR_H */
