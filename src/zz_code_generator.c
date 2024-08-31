#include "zz_code_generator.h"

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

static rt_s zz_code_generator_generate_expression(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value);

static rt_s zz_code_generator_handle_llvm_error_message(rt_char8 *llvm_error)
{
	rt_s ret;

	if (llvm_error) {
		if (RT_UNLIKELY(!rt_console8_write_error(llvm_error, RT_ENCODING_SYSTEM_DEFAULT)))
			goto error;
		if (RT_UNLIKELY(!rt_console8_write_error("\n", RT_ENCODING_SYSTEM_DEFAULT)))
			goto error;
	}

	ret = RT_OK;
free:
	if (llvm_error)
		LLVMDisposeMessage(llvm_error);
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_code_generator_generate_number(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMValueRef *llvm_value)
{
	*llvm_value = LLVMConstInt(LLVMInt32TypeInContext(llvm_context), node->u.number.value, RT_TRUE);
	return RT_OK;
}

static rt_s zz_code_generator_generate_unary_operator(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value)
{
	LLVMValueRef operand;
	rt_s ret;

	if (RT_UNLIKELY(!zz_code_generator_generate_expression(node->u.unary_operator.operand, llvm_context, llvm_module, llvm_builder, &operand)))
		goto error;

	switch (node->u.unary_operator.unary_operator) {
	case ZZ_UNARY_OPERATOR_NEGATE:
		*llvm_value = LLVMBuildNeg(llvm_builder, operand, "neg");
		break;
	default:
		rt_error_set_last(RT_ERROR_BAD_ARGUMENTS);
		goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_code_generator_generate_binary_operator(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value)
{
	LLVMValueRef left_side_operand;
	LLVMValueRef right_side_operand;

	if (RT_UNLIKELY(!zz_code_generator_generate_expression(node->u.binary_operator.left, llvm_context, llvm_module, llvm_builder, &left_side_operand)))
		goto error;
	
	if (RT_UNLIKELY(!zz_code_generator_generate_expression(node->u.binary_operator.right, llvm_context, llvm_module, llvm_builder, &right_side_operand)))
		goto error;

	switch (node->u.binary_operator.binary_operator) {
	case ZZ_BINARY_OPERATOR_ADD:
		*llvm_value = LLVMBuildAdd(llvm_builder, left_side_operand, right_side_operand, "add");
		break;
	case ZZ_BINARY_OPERATOR_SUBTRACT:
		*llvm_value = LLVMBuildSub(llvm_builder, left_side_operand, right_side_operand, "sub");
		break;
	case ZZ_BINARY_OPERATOR_MULTIPLY:
		*llvm_value = LLVMBuildMul(llvm_builder, left_side_operand, right_side_operand, "mul");
		break;
	case ZZ_BINARY_OPERATOR_DIVIDE:
		*llvm_value = LLVMBuildSDiv(llvm_builder, left_side_operand, right_side_operand, "div");
		break;
	case ZZ_BINARY_OPERATOR_MODULO:
		*llvm_value = LLVMBuildSRem(llvm_builder, left_side_operand, right_side_operand, "mod");
		break;
	default:
		rt_error_set_last(RT_ERROR_BAD_ARGUMENTS);
		goto error;
	}

	rt_s ret;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_code_generator_generate_expression(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value)
{
	rt_s ret;

	switch (node->type) {
	case ZZ_AST_NODE_TYPE_NUMBER:
		if (RT_UNLIKELY(!zz_code_generator_generate_number(node, llvm_context, llvm_value)))
			goto error;
		break;
	case ZZ_AST_NODE_TYPE_UNARY_OPERATOR:
		if (RT_UNLIKELY(!zz_code_generator_generate_unary_operator(node, llvm_context, llvm_module, llvm_builder, llvm_value)))
			goto error;
		break;
	case ZZ_AST_NODE_TYPE_BINARY_OPERATOR:
		if (RT_UNLIKELY(!zz_code_generator_generate_binary_operator(node, llvm_context, llvm_module, llvm_builder, llvm_value)))
			goto error;
		break;
	default:
		rt_error_set_last(RT_ERROR_BAD_ARGUMENTS);
		goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_code_generator_generate_function(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder)
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

	if (RT_UNLIKELY(!zz_code_generator_generate_expression(node->u.function.body, llvm_context, llvm_module, llvm_builder, &llvm_body_value)))
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

static rt_s zz_code_generator_generate_do(struct zz_ast_node *root, rt_char *output_file_path, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder)
{
	LLVMTargetRef target;
	rt_char8 *llvm_error;
	rt_char8 output_file_path8[RT_FILE_PATH_SIZE];
	rt_un output_file_path8_size;
	rt_char8 *output;
	rt_s ret;

	/* TODO: For now, we assume that the root is a function. Later it will be a module. */
	if (RT_UNLIKELY(!zz_code_generator_generate_function(root, llvm_context, llvm_module, llvm_builder)))
		goto error;

	if (RT_UNLIKELY(LLVMInitializeNativeTarget())) {
		rt_error_set_last(RT_ERROR_FUNCTION_FAILED);
		goto error;
	}
	if (RT_UNLIKELY(LLVMInitializeNativeAsmPrinter())) {
		rt_error_set_last(RT_ERROR_FUNCTION_FAILED);
		goto error;
	}

	if (RT_UNLIKELY(LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &llvm_error))) {
		zz_code_generator_handle_llvm_error_message(llvm_error);
		rt_error_set_last(RT_ERROR_FUNCTION_FAILED);
		goto error;
	}

	LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
		target,
		LLVMGetDefaultTargetTriple(),
		LLVMGetHostCPUName(),
		LLVMGetHostCPUFeatures(),
		LLVMCodeGenLevelDefault,
		LLVMRelocDefault,
		LLVMCodeModelDefault
	);

	LLVMSetModuleDataLayout(llvm_module, LLVMCreateTargetDataLayout(target_machine));

	if (RT_UNLIKELY(!rt_encoding_encode(output_file_path, rt_char_get_size(output_file_path), RT_ENCODING_SYSTEM_DEFAULT, output_file_path8, RT_FILE_PATH_SIZE, RT_NULL, RT_NULL, &output, &output_file_path8_size, RT_NULL)))
		goto error;

	if (RT_UNLIKELY(LLVMTargetMachineEmitToFile(target_machine, llvm_module, output_file_path8, LLVMObjectFile, &llvm_error))) {
		zz_code_generator_handle_llvm_error_message(llvm_error);
		rt_error_set_last(RT_ERROR_FUNCTION_FAILED);
		goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

rt_s zz_code_generator_generate(struct zz_ast_node *root, rt_char *output_file_path)
{
	LLVMContextRef llvm_context;
	LLVMModuleRef llvm_module;
	LLVMBuilderRef llvm_builder;
	rt_s ret;

	llvm_context = LLVMContextCreate();
	llvm_module = LLVMModuleCreateWithName("stc_module");
	llvm_builder = LLVMCreateBuilderInContext(llvm_context);

	if (RT_UNLIKELY(!zz_code_generator_generate_do(root, output_file_path, llvm_context, llvm_module, llvm_builder)))
		goto error;

	/* TODO: Temporary. Maybe I should add a flag parameter so that the IR can be displayed or put in a file. */
	LLVMDumpModule(llvm_module);

	ret = RT_OK;
free:
	LLVMDisposeBuilder(llvm_builder);
	LLVMDisposeModule(llvm_module);
	LLVMContextDispose(llvm_context);

	return ret;

error:
	ret = RT_FAILED;
	goto free;
}
