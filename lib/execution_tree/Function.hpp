#ifndef EXECUTION_TREE_FUNCTION_HPP
#define EXECUTION_TREE_FUNCTION_HPP

#include <cstddef>
#include <expected>
#include <memory>
#include <stdexcept>

#include "ExecutionResult.hpp"
#include "IExecutable.hpp"
#include "IFunctionExecutable.hpp"
#include "lib/runtime/FunctionId.hpp"

namespace ovum::vm::execution_tree {

class Function : public IFunctionExecutable {
public:
  Function(runtime::FunctionId id, size_t arity, std::unique_ptr<IExecutable> body);

  std::expected<ExecutionResult, std::runtime_error> Execute(PassedExecutionData& execution_data) override;

  [[nodiscard]] runtime::FunctionId GetId() const override;
  [[nodiscard]] size_t GetArity() const override;
  [[nodiscard]] size_t GetTotalActionCount() const override;
  [[nodiscard]] size_t GetExecutionCount() const override;

private:
  runtime::FunctionId id_;
  size_t arity_{};
  size_t total_action_count_{};
  size_t execution_count_{};
  std::unique_ptr<IExecutable> body_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_FUNCTION_HPP
