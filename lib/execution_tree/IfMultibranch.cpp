#include "IfMultibranch.hpp"

#include "PassedExecutionData.hpp"

namespace ovum::vm::execution_tree {

IfMultibranch::IfMultibranch() = default;

void IfMultibranch::AddBranch(std::unique_ptr<ConditionalExecution> branch) {
  branches_.emplace_back(std::move(branch));
}

std::expected<ExecutionResult, std::runtime_error> IfMultibranch::Execute(PassedExecutionData& execution_data) {
  for (const auto& branch : branches_) {
    const std::expected<ExecutionResult, std::runtime_error> result = branch->Execute(execution_data);

    if (!result.has_value()) {
      return result;
    }

    const ExecutionResult execution_result = result.value();

    if (execution_result != ExecutionResult::kNormal) {
      return execution_result;
    }
  }

  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree
