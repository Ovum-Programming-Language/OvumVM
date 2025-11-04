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

  runtime::StackFrame local_frame{};
  local_frame.local_variables.reserve(arity_);

  for (size_t i = 0; i < arity_; ++i) {
    local_frame.local_variables.emplace_back(runtime_memory.machine_stack.top());
    runtime_memory.machine_stack.pop();
  }

  std::reverse(local_frame.local_variables.begin(), local_frame.local_variables.end());

  runtime_memory.stack_frames.push(std::move(local_frame));

  const std::expected<ExecutionResult, std::runtime_error> result = block_->Execute(runtime_memory);

  if (!result.has_value()) {
    return result;
  }

  ++execution_count_;
  total_action_count_ += runtime_memory.stack_frames.top().action_count;
  runtime_memory.stack_frames.pop();
  const ExecutionResult execution_result = result.value();

  if (execution_result == ExecutionResult::kReturn) {
    return ExecutionResult::kNormal;
  }

  return execution_result;
}

runtime::FunctionId Function::GetId() const {
  return id_;
}

size_t Function::GetArity() const {
  return arity_;
}

size_t Function::GetTotalActionCount() const {
  return total_action_count_;
}

size_t Function::GetExecutionCount() const {
  return execution_count_;
}

} // namespace ovum::vm::execution_tree
