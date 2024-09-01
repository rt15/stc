#include "code_generator/zz_code_generator.h"

#include "code_generator/zz_function_generator.h"

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

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

static rt_s zz_code_generator_generate_do(struct zz_ast_node *root, rt_char *output_file_path, LLVMContextRef llvm_context, LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder)
{
	LLVMTargetRef target;
	rt_char8 *llvm_error;
	rt_char8 output_file_path8[RT_FILE_PATH_SIZE];
	rt_un output_file_path8_size;
	rt_char8 *output;
	rt_s ret;

	/* TODO: For now, we assume that the root is a function. Later it will be a module. */
	if (RT_UNLIKELY(!zz_function_generator_generate(root, llvm_context, llvm_module, llvm_builder)))
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
