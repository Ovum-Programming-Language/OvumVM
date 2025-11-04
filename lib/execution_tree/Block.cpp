#include "Block.hpp"

#include "PassedExecutionData.hpp"

namespace ovum::vm::execution_tree {

Block::Block() = default;

void Block::AddStatement(std::unique_ptr<IExecutable> statement) {
  statements_.emplace_back(std::move(statement));
}

std::expected<ExecutionResult, std::runtime_error> Block::Execute(PassedExecutionData& execution_data) {
  for (const auto& statement : statements_) {
    const std::expected<ExecutionResult, std::runtime_error> result = statement->Execute(execution_data);

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
