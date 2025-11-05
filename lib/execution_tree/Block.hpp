#ifndef EXECUTION_TREE_BLOCK_HPP
#define EXECUTION_TREE_BLOCK_HPP

#include <memory>
#include <vector>

#include "IExecutable.hpp"

namespace ovum::vm::execution_tree {

class Block : public IExecutable {
public:
  Block();

  void AddStatement(std::unique_ptr<IExecutable> statement);

  std::expected<ExecutionResult, std::runtime_error> Execute(PassedExecutionData& execution_data) override;

private:
  std::vector<std::unique_ptr<IExecutable>> statements_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_BLOCK_HPP
