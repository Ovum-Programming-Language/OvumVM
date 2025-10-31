#ifndef EXECUTION_TREE_WHILEEXECUTION_HPP
#define EXECUTION_TREE_WHILEEXECUTION_HPP

#include <expected>
#include <memory>
#include <stdexcept>

#include "ExecutionResult.hpp"
#include "IExecutable.hpp"

namespace ovum::vm::execution_tree {

class WhileExecution : public IExecutable {
public:
  WhileExecution(std::unique_ptr<IExecutable> condition_block, std::unique_ptr<IExecutable> execution_block);

  std::expected<ExecutionResult, std::runtime_error> Execute(runtime::RuntimeMemory& runtime_memory) override;

private:
  std::unique_ptr<IExecutable> condition_block_;
  std::unique_ptr<IExecutable> execution_block_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_WHILEEXECUTION_HPP
