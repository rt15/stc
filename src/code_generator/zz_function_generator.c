#include "code_generator/zz_function_generator.h"

#include "code_generator/zz_expression_generator.h"

rt_s zz_function_generator_generate(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder)
{
	LLVMValueRef llvm_body_value;
	LLVMTypeRef main_function_return_type;
	LLVMTypeRef main_function_param_types[] = { };
	LLVMTypeRef main_function_type;
	LLVMValueRef main_function;
	LLVMBasicBlockRef main_function_entry;
	rt_s ret;

	if (node->type != ZZ_AST_NODE_TYPE_FUNCTION) {
		goto error;
	}

	if (RT_UNLIKELY(!zz_expression_generator_generate(node->u.function.body, llvm_context, llvm_module, llvm_builder, &llvm_body_value)))
		goto error;

	main_function_return_type = LLVMInt32TypeInContext(llvm_context);
	main_function_type = LLVMFunctionType(main_function_return_type, main_function_param_types, 0, RT_FALSE);
	main_function = LLVMAddFunction(llvm_module, "main", main_function_type);
	main_function_entry = LLVMAppendBasicBlockInContext(llvm_context, main_function, "entry");
	LLVMPositionBuilderAtEnd(llvm_builder, main_function_entry);
	LLVMBuildRet(llvm_builder, llvm_body_value);

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}
