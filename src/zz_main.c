#define RT_DEFINE_USE_CRT

#include <rpr.h>
#include <rpr_main.h>

#include "zz_lexer.h"
#include "zz_parser.h"
#include "zz_ast.h"
#include "zz_code_generator.h"

static rt_s zz_display_help(rt_s ret)
{
	rt_b error = !ret;

	if (!rt_console_write(_R("stc <FILE>\n"), error))
		ret = RT_FAILED;

	return ret;
}

static rt_s zz_stc_execute_node(struct zz_ast_node *ast_node, rt_n *result)
{
	rt_n left_value;
	rt_n right_value;
	rt_n value;
	rt_s ret;

	switch (ast_node->type) {
	case ZZ_AST_NODE_TYPE_NUMBER:
		*result = ast_node->u.number.value;
		break;
	case ZZ_AST_NODE_TYPE_BINARY_OPERATOR:
		if (RT_UNLIKELY(!zz_stc_execute_node(ast_node->u.binary_operator.left, &left_value)))
			goto error;

		if (RT_UNLIKELY(!zz_stc_execute_node(ast_node->u.binary_operator.right, &right_value)))
			goto error;

		switch (ast_node->u.binary_operator.binary_operator) {
		case ZZ_BINARY_OPERATOR_ADD:
			*result = left_value + right_value;
			break;
		case ZZ_BINARY_OPERATOR_SUBTRACT:
			*result = left_value - right_value;
			break;
		case ZZ_BINARY_OPERATOR_MULTIPLY:
			*result = left_value * right_value;
			break;
		case ZZ_BINARY_OPERATOR_DIVIDE:
			*result = left_value / right_value;
			break;
		case ZZ_BINARY_OPERATOR_MODULO:
			*result = left_value % right_value;
			break;
		default:
			rt_error_set_last(RT_ERROR_BAD_ARGUMENTS);
			goto error;
		}
		break;
	case ZZ_AST_NODE_TYPE_UNARY_OPERATOR:
		if (RT_UNLIKELY(!zz_stc_execute_node(ast_node->u.unary_operator.operand, &value)))
			goto error;
		switch (ast_node->u.unary_operator.unary_operator) {
		case ZZ_UNARY_OPERATOR_NEGATE:
			*result = -value;
			break;
		default:
			rt_error_set_last(RT_ERROR_BAD_ARGUMENTS);
			goto error;
		}
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

static rt_s zz_stc_execute(struct zz_ast_node *root)
{
	rt_n current_value;
	rt_char buffer[RT_CHAR_HALF_BIG_STRING_SIZE];
	rt_un buffer_size;
	rt_s ret;

	if (RT_UNLIKELY(!zz_stc_execute_node(root, &current_value)))
		goto error;

	buffer_size = 8;
	if (RT_UNLIKELY(!rt_char_copy(_R("Result: "), buffer_size, buffer, RT_CHAR_HALF_BIG_STRING_SIZE)))
		goto error;

	if (RT_UNLIKELY(!rt_char_append_n(current_value, 10, buffer, RT_CHAR_HALF_BIG_STRING_SIZE, &buffer_size)))
		goto error;

	if (RT_UNLIKELY(!rt_char_append_char(_R('\n'), buffer, RT_CHAR_HALF_BIG_STRING_SIZE, &buffer_size)))
		goto error;

	if (RT_UNLIKELY(!rt_console_write_str_with_size(buffer, buffer_size)))
		goto error;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_stc_with_lexer(struct zz_lexer *lexer, rt_char *output_file_path, struct rt_heap *heap)
{
	void *ast_nodes_list = RT_NULL;
	struct zz_ast_node *root;
	rt_s ret;

	if (RT_UNLIKELY(!rt_list_create(&ast_nodes_list, 0, sizeof(struct zz_ast_node), 16384, 0, heap)))
		goto error;

	if (RT_UNLIKELY(!zz_parser_parse(lexer, &ast_nodes_list, &root))) {
		rt_error_message_write_last(_R("Compilation failed: "));
		goto error;
	}

	if (RT_UNLIKELY(!zz_stc_execute(root))) {
		rt_error_message_write_last(_R("Execution failed: "));
		goto error;
	}

	if (RT_UNLIKELY(!zz_code_generator_generate(root, output_file_path))) {
		rt_error_message_write_last(_R("Code generation failed: "));
		goto error;
	}

	ret = RT_OK;
free:
	if (RT_UNLIKELY(!rt_list_free((void**)&ast_nodes_list) && ret))
		goto error;
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_stc_with_char(rt_char *input, rt_char *output_file_path, struct rt_heap *heap)
{
	struct zz_lexer lexer;
	rt_s ret;

	lexer.input = input;

	if (RT_UNLIKELY(!zz_stc_with_lexer(&lexer, output_file_path, heap)))
		goto error;

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_stc_with_char8(rt_char8 *input, rt_un input_size, rt_char *output_file_path, struct rt_heap *heap)
{
	void *heap_buffer = RT_NULL;
	rt_un heap_buffer_capacity = 0;
	rt_char *output;
	rt_un output_size;
	rt_s ret;

	if (RT_UNLIKELY(!rt_encoding_decode(input, input_size, RT_ENCODING_UTF_8, RT_NULL, 0, &heap_buffer, &heap_buffer_capacity, &output, &output_size, heap)))
		goto error;

	if (RT_UNLIKELY(!zz_stc_with_char(output, output_file_path, heap)))
		goto error;

	ret = RT_OK;
free:
	if (heap_buffer) {
		if (RT_UNLIKELY(!heap->free(heap, &heap_buffer) && ret))
			goto error;
	}
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_stc_with_heap(const rt_char *input_file_path, struct rt_heap *heap)
{
	void *heap_buffer = RT_NULL;
	rt_un heap_buffer_capacity = 0;
	rt_char8 *output;
	rt_un output_size;
	rt_char output_file_path[RT_FILE_PATH_SIZE];
	rt_un output_file_path_size;
	rt_s ret;

	if (RT_UNLIKELY(!rt_small_file_read(input_file_path, RT_NULL, 0, &heap_buffer, &heap_buffer_capacity, &output, &output_size, heap)))
		goto error;

	if (RT_UNLIKELY(!rt_file_path_get_name(input_file_path, rt_char_get_size(input_file_path), output_file_path, RT_FILE_PATH_SIZE, &output_file_path_size)))
		goto error;
	
	if (RT_UNLIKELY(!rt_char_ends_with(output_file_path, output_file_path_size, _R(".stc"), 4))) {
		rt_error_set_last(RT_ERROR_BAD_ARGUMENTS);
		goto error;
	}
	output_file_path[output_file_path_size - 3] = _R('o');
	output_file_path[output_file_path_size - 2] = 0;

	if (RT_UNLIKELY(!zz_stc_with_char8(output, output_size, output_file_path, heap)))
		goto error;

	ret = RT_OK;
free:
	if (heap_buffer) {
		if (RT_UNLIKELY(!heap->free(heap, &heap_buffer) && ret))
			goto error;
	}
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_stc(const rt_char *input_file_path)
{
	struct rt_runtime_heap runtime_heap;
	rt_b runtime_heap_created = RT_FALSE;
	rt_s ret;

	if (RT_UNLIKELY(!rt_runtime_heap_create(&runtime_heap)))
		goto error;
	runtime_heap_created = RT_TRUE;

	if (RT_UNLIKELY(!zz_stc_with_heap(input_file_path, &runtime_heap.heap)))
		goto error;

	ret = RT_OK;
free:
	if (runtime_heap_created) {
		runtime_heap_created = RT_FALSE;
		if (RT_UNLIKELY(!runtime_heap.heap.close(&runtime_heap.heap) && ret))
			goto error;
	}
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

static rt_s zz_main(rt_un argc, const rt_char *argv[])
{
	rt_un arg_size;
	rt_s ret;

	if (argc == 2) {
		arg_size = rt_char_get_size(argv[1]);
		if (rt_char_equals(argv[1], arg_size, _R("--help"), 6) ||
			rt_char_equals(argv[1], arg_size, _R("-h"), 2) ||
			rt_char_equals(argv[1], arg_size, _R("/?"), 2)) {

			if (RT_UNLIKELY(!zz_display_help(RT_OK)))
				goto error;
		} else {
			if (RT_UNLIKELY(!zz_stc(argv[1])))
				goto error;
		}
	} else {
		if (!zz_display_help(RT_FAILED))
			goto error;
	}

	ret = RT_OK;
free:
	return ret;

error:
	ret = RT_FAILED;
	goto free;
}

rt_un16 rpr_main(rt_un argc, const rt_char *argv[])
{
	int ret;

	if (zz_main(argc, argv))
		ret = 0;
	else
		ret = 1;
	return ret;
}
