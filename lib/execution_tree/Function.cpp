#include "Function.hpp"

namespace ovum::vm::execution_tree {

Function::Function(runtime::FunctionId id, size_t arity, std::unique_ptr<IExecutable> body) :
    id_(std::move(id)), arity_(arity), body_(std::move(body)) {
}

std::expected<ExecutionResult, std::runtime_error> Function::Execute(PassedExecutionData& execution_data) {
  if (execution_data.memory.machine_stack.size() < arity_) {
    return std::unexpected(std::runtime_error("Function " + id_ + ": insufficient arguments on stack (expected " +
                                              std::to_string(arity_) + ", got " +
                                              std::to_string(execution_data.memory.machine_stack.size()) + ")"));
  }

  runtime::StackFrame local_frame{};
  local_frame.local_variables.reserve(arity_);

  for (size_t i = 0; i < arity_; ++i) {
    local_frame.local_variables.emplace_back(execution_data.memory.machine_stack.top());
    execution_data.memory.machine_stack.pop();
  }

  execution_data.memory.stack_frames.push(std::move(local_frame));

  const std::expected<ExecutionResult, std::runtime_error> result = body_->Execute(execution_data);

  if (!result.has_value()) {
    execution_data.memory.stack_frames.pop();
    return result;
  }

  ++execution_count_;
  total_action_count_ += execution_data.memory.stack_frames.top().action_count;
  execution_data.memory.stack_frames.pop();
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
