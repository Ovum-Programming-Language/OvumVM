#include "Executor.hpp"

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/IFunctionExecutable.hpp"

namespace ovum::vm::executor {

const std::string Executor::kMainFunctionName = "_Global_Main_StringArray";

Executor::Executor(execution_tree::PassedExecutionData& execution_data) : execution_data_(execution_data) {
}

std::expected<execution_tree::ExecutionResult, std::runtime_error> Executor::RunProgram(
    const std::unique_ptr<execution_tree::Block>& init_static) {
  if (!init_static) {
    return std::unexpected(std::runtime_error("Excution failed: init-static block is null"));
  }

  const std::expected<execution_tree::ExecutionResult, std::runtime_error> block_result =
      init_static->Execute(execution_data_);

  if (!block_result.has_value()) {
    return block_result;
  }

  const std::expected<execution_tree::IFunctionExecutable*, std::runtime_error> main_function =
      execution_data_.function_repository.GetByName(kMainFunctionName);

  if (!main_function.has_value()) {
    return std::unexpected(std::runtime_error("Execution failed: main function '" + kMainFunctionName + "' not found"));
  }

  return main_function.value()->Execute(execution_data_);
}

} // namespace ovum::vm::executor
