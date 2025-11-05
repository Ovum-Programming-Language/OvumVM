#ifndef EXECUTION_TREE_IFUNCTIONEXECUTABLE_HPP
#define EXECUTION_TREE_IFUNCTIONEXECUTABLE_HPP

#include <cstddef>

#include "IExecutable.hpp"
#include "lib/runtime/FunctionId.hpp"

namespace ovum::vm::execution_tree {

class IFunctionExecutable : public IExecutable { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IFunctionExecutable() = default;

  [[nodiscard]] virtual runtime::FunctionId GetId() const = 0;
  [[nodiscard]] virtual size_t GetArity() const = 0;
  [[nodiscard]] virtual size_t GetTotalActionCount() const = 0;
  [[nodiscard]] virtual size_t GetExecutionCount() const = 0;
};

} // namespace ovum::vm::execution_tree
#endif // EXECUTION_TREE_IFUNCTIONEXECUTABLE_HPP
