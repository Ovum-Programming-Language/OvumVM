#ifndef EXECUTION_TREE_JITCOMPILINGFUNCTION_HPP
#define EXECUTION_TREE_JITCOMPILINGFUNCTION_HPP

#include <cstddef>
#include <expected>
#include <memory>
#include <stdexcept>

#include "ExecutionConcepts.hpp"
#include "ExecutionResult.hpp"
#include "IFunctionExecutable.hpp"
#include "PassedExecutionData.hpp"
#include "lib/executor/IJitExecutor.hpp"

namespace ovum::vm::execution_tree {

template<ExecutableFunction ExecutableFunctionType>
class JitCompilingFunction : public IFunctionExecutable {
public:
  JitCompilingFunction(std::unique_ptr<executor::IJitExecutor> executor,
                       const ExecutableFunctionType& function,
                       size_t jit_action_boundary) :
      executor_(std::move(executor)), function_(function), jit_action_boundary_(jit_action_boundary) {
  }

  std::expected<ExecutionResult, std::runtime_error> Execute(PassedExecutionData& execution_data) override {
    if (function_.GetTotalActionCount() > jit_action_boundary_) {
      if (executor_->TryCompile()) {
        const std::expected<void, std::runtime_error> jit_result = executor_->Run(execution_data.memory.machine_stack);

        if (jit_result.has_value()) {
          return ExecutionResult::kNormal;
        }
      }
    }

    return function_.Execute(execution_data);
  }

  [[nodiscard]] runtime::FunctionId GetId() const override {
    return function_.GetId();
  }

  [[nodiscard]] size_t GetArity() const override {
    return function_.GetArity();
  }

  [[nodiscard]] size_t GetTotalActionCount() const override {
    return function_.GetTotalActionCount();
  }

  [[nodiscard]] size_t GetExecutionCount() const override {
    return function_.GetExecutionCount();
  }

private:
  std::unique_ptr<executor::IJitExecutor> executor_;
  ExecutableFunctionType function_;
  size_t jit_action_boundary_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_JITCOMPILINGFUNCTION_HPP
