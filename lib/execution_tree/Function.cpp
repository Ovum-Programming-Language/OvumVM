#include "Function.hpp"

#include <algorithm>

namespace ovum::vm::execution_tree {

Function::Function(runtime::FunctionId id, size_t arity, std::unique_ptr<Block> block) :
    id_(std::move(id)), arity_(arity), block_(std::move(block)) {
}

std::expected<ExecutionResult, std::runtime_error> Function::Execute(runtime::RuntimeMemory& runtime_memory) {
  if (runtime_memory.machine_stack.size() < arity_) {
    return std::unexpected(std::runtime_error("Function " + id_ + ": insufficient arguments on stack (expected " +
                                              std::to_string(arity_) + ", got " +
                                              std::to_string(runtime_memory.machine_stack.size()) + ")"));
  }

  runtime::VariableCollection local_frame;
  local_frame.reserve(arity_);

  for (size_t i = 0; i < arity_; ++i) {
    local_frame.push_back(runtime_memory.machine_stack.top());
    runtime_memory.machine_stack.pop();
  }

  std::reverse(local_frame.begin(), local_frame.end());

  runtime_memory.local_variables.push(std::move(local_frame));

  const std::expected<ExecutionResult, std::runtime_error> result = block_->Execute(runtime_memory);

  runtime_memory.local_variables.pop();

  if (!result.has_value()) {
    return result;
  }

  const ExecutionResult execution_result = result.value();

  if (execution_result == ExecutionResult::kReturn) {
    return ExecutionResult::kNormal;
  }

  return execution_result;
}

} // namespace ovum::vm::execution_tree
