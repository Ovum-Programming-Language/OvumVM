#include "ConditionalExecution.hpp"

#include <variant>

namespace ovum::vm::execution_tree {

ConditionalExecution::ConditionalExecution(std::unique_ptr<IExecutable> condition_block,
                                           std::unique_ptr<IExecutable> execution_block) :
    condition_block_(std::move(condition_block)), execution_block_(std::move(execution_block)) {
}

std::expected<ExecutionResult, std::runtime_error> ConditionalExecution::Execute(
    runtime::RuntimeMemory& runtime_memory) {
  const std::expected<ExecutionResult, std::runtime_error> condition_result = condition_block_->Execute(runtime_memory);

  if (!condition_result.has_value()) {
    return condition_result;
  }

  if (condition_result.value() != ExecutionResult::kNormal) {
    return condition_result;
  }

  if (runtime_memory.machine_stack.empty()) {
    return std::unexpected(
        std::runtime_error("ConditionalExecution: machine stack is empty after condition execution"));
  }

  const runtime::Variable top_value = runtime_memory.machine_stack.top();
  runtime_memory.machine_stack.pop();

  if (!std::holds_alternative<bool>(top_value)) {
    return std::unexpected(std::runtime_error("ConditionalExecution: condition result is not a boolean"));
  }

  const bool condition_bool = std::get<bool>(top_value);

  if (!condition_bool) {
    return ExecutionResult::kNormal;
  }

  return execution_block_->Execute(runtime_memory);
}

} // namespace ovum::vm::execution_tree
