#ifndef ZZ_EXPRESSION_GENERATOR_H
#define ZZ_EXPRESSION_GENERATOR_H

#include <rpr.h>

#include "ast/zz_ast.h"

#include "llvm-c/Core.h"

rt_s zz_expression_generator_generate(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value);

#endif /* ZZ_EXPRESSION_GENERATOR_H */
