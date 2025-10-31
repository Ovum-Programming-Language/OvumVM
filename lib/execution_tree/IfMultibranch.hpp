#ifndef EXECUTION_TREE_IFMULTIBRANCH_HPP
#define EXECUTION_TREE_IFMULTIBRANCH_HPP

#include <expected>
#include <memory>
#include <stdexcept>
#include <vector>

#include "ConditionalExecution.hpp"
#include "ExecutionResult.hpp"
#include "IExecutable.hpp"

namespace ovum::vm::execution_tree {

class IfMultibranch : public IExecutable {
public:
  IfMultibranch();

  void AddBranch(std::unique_ptr<ConditionalExecution> branch);

  std::expected<ExecutionResult, std::runtime_error> Execute(runtime::RuntimeMemory& runtime_memory) override;

private:
  std::vector<std::unique_ptr<ConditionalExecution>> branches_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_IFMULTIBRANCH_HPP
