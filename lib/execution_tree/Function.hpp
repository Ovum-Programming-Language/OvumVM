#ifndef EXECUTION_TREE_FUNCTION_HPP
#define EXECUTION_TREE_FUNCTION_HPP

#include <cstddef>
#include <expected>
#include <memory>
#include <stdexcept>

#include "Block.hpp"
#include "ExecutionResult.hpp"
#include "IExecutable.hpp"
#include "lib/runtime/FunctionId.hpp"

namespace ovum::vm::execution_tree {

class Function : public IExecutable {
public:
  Function(runtime::FunctionId id, size_t arity, std::unique_ptr<Block> block);

  std::expected<ExecutionResult, std::runtime_error> Execute(runtime::RuntimeMemory& runtime_memory) override;

private:
  runtime::FunctionId id_;
  size_t arity_;
  std::unique_ptr<Block> block_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_FUNCTION_HPP
