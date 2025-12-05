#ifndef EXECUTION_TREE_IFMULTIBRANCH_HPP
#define EXECUTION_TREE_IFMULTIBRANCH_HPP

#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "Block.hpp"
#include "ConditionalExecution.hpp"
#include "ExecutionResult.hpp"
#include "IExecutable.hpp"

namespace ovum::vm::execution_tree {

class IfMultibranch : public IExecutable {
public:
  IfMultibranch();

  void AddBranch(std::unique_ptr<ConditionalExecution> branch);
  void SetElseBlock(std::unique_ptr<Block> else_block);

  std::expected<ExecutionResult, std::runtime_error> Execute(PassedExecutionData& execution_data) override;

private:
  std::vector<std::unique_ptr<ConditionalExecution>> branches_;
  std::optional<std::unique_ptr<Block>> else_block_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_IFMULTIBRANCH_HPP
