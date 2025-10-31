#ifndef EXECUTION_TREE_IEXECUTABLE_HPP
#define EXECUTION_TREE_IEXECUTABLE_HPP

#include <expected>
#include <stdexcept>

#include "ExecutionResult.hpp"
#include "lib/runtime/RuntimeMemory.hpp"

namespace ovum::vm::execution_tree {

class IExecutable { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IExecutable() = default;

  virtual std::expected<ExecutionResult, std::runtime_error> Execute(runtime::RuntimeMemory& runtime_memory) = 0;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_IEXECUTABLE_HPP
