#include "IfMultibranch.hpp"

namespace ovum::vm::execution_tree {

IfMultibranch::IfMultibranch() = default;

void IfMultibranch::AddBranch(std::unique_ptr<ConditionalExecution> branch) {
  branches_.emplace_back(std::move(branch));
}

void IfMultibranch::SetElseBlock(std::unique_ptr<Block> else_block) {
  else_block_ = std::move(else_block);
}

std::expected<ExecutionResult, std::runtime_error> IfMultibranch::Execute(PassedExecutionData& execution_data) {
  for (const auto& branch : branches_) {
    const std::expected<ExecutionResult, std::runtime_error> result = branch->Execute(execution_data);

    if (!result.has_value()) {
      return result;
    }

    const ExecutionResult execution_result = result.value();

    if (execution_result != ExecutionResult::kConditionFalse) {
      return execution_result;
    }
  }

  if (else_block_.has_value()) {
    return else_block_.value()->Execute(execution_data);
  }

  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree
