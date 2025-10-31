#include "WhileExecution.hpp"

#include <variant>

namespace ovum::vm::execution_tree {

WhileExecution::WhileExecution(std::unique_ptr<IExecutable> condition_block,
                               std::unique_ptr<IExecutable> execution_block) :
    condition_block_(std::move(condition_block)), execution_block_(std::move(execution_block)) {
}

std::expected<ExecutionResult, std::runtime_error> WhileExecution::Execute(runtime::RuntimeMemory& runtime_memory) {
  while (true) {
    const std::expected<ExecutionResult, std::runtime_error> condition_result =
        condition_block_->Execute(runtime_memory);

    if (!condition_result.has_value()) {
      return condition_result;
    }

    if (condition_result.value() != ExecutionResult::kNormal) {
      return condition_result;
    }

    if (runtime_memory.machine_stack.empty()) {
      return std::unexpected(std::runtime_error("WhileExecution: machine stack is empty after condition execution"));
    }

    const runtime::Variable top_value = runtime_memory.machine_stack.top();
    runtime_memory.machine_stack.pop();

    if (!std::holds_alternative<bool>(top_value)) {
      return std::unexpected(std::runtime_error("WhileExecution: condition result is not a boolean"));
    }

    const bool condition_bool = std::get<bool>(top_value);

    if (!condition_bool) {
      return ExecutionResult::kNormal;
    }

    const std::expected<ExecutionResult, std::runtime_error> execution_result =
        execution_block_->Execute(runtime_memory);

    if (!execution_result.has_value()) {
      return execution_result;
    }

    const ExecutionResult result = execution_result.value();

    if (result == ExecutionResult::kBreak) {
      return ExecutionResult::kNormal;
    }

    if (result == ExecutionResult::kContinue) {
      continue;
    }

    if (result == ExecutionResult::kReturn) {
      return ExecutionResult::kReturn;
    }
  }
}

} // namespace ovum::vm::execution_tree
