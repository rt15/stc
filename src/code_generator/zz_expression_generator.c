#include "code_generator/zz_expression_generator.h"

static rt_s zz_expression_generator_generate_number(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMValueRef *llvm_value)
{
	*llvm_value = LLVMConstInt(LLVMInt32TypeInContext(llvm_context), node->u.number.value, RT_TRUE);
	return RT_OK;
}

static rt_s zz_expression_generator_generate_unary_operator(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value)
{
	LLVMValueRef operand;
	rt_s ret;

	if (RT_UNLIKELY(!zz_expression_generator_generate(node->u.unary_operator.operand, llvm_context, llvm_module, llvm_builder, &operand)))
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

static rt_s zz_expression_generator_generate_binary_operator(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value)
{
	LLVMValueRef left_side_operand;
	LLVMValueRef right_side_operand;

	if (RT_UNLIKELY(!zz_expression_generator_generate(node->u.binary_operator.left, llvm_context, llvm_module, llvm_builder, &left_side_operand)))
		goto error;
	
	if (RT_UNLIKELY(!zz_expression_generator_generate(node->u.binary_operator.right, llvm_context, llvm_module, llvm_builder, &right_side_operand)))
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

rt_s zz_expression_generator_generate(struct zz_ast_node *node, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, LLVMValueRef *llvm_value)
{
	rt_s ret;

	switch (node->type) {
	case ZZ_AST_NODE_TYPE_NUMBER:
		if (RT_UNLIKELY(!zz_expression_generator_generate_number(node, llvm_context, llvm_value)))
			goto error;
		break;
	case ZZ_AST_NODE_TYPE_UNARY_OPERATOR:
		if (RT_UNLIKELY(!zz_expression_generator_generate_unary_operator(node, llvm_context, llvm_module, llvm_builder, llvm_value)))
			goto error;
		break;
	case ZZ_AST_NODE_TYPE_BINARY_OPERATOR:
		if (RT_UNLIKELY(!zz_expression_generator_generate_binary_operator(node, llvm_context, llvm_module, llvm_builder, llvm_value)))
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
