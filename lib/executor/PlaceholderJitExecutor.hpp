#ifndef EXECUTOR_PLACEHOLDERJITEXECUTOR_HPP
#define EXECUTOR_PLACEHOLDERJITEXECUTOR_HPP

#include "IJitExecutor.hpp"

#include "lib/execution_tree/PassedExecutionData.hpp"

namespace ovum::vm::executor {

class PlaceholderJitExecutor : public IJitExecutor {
public:
  PlaceholderJitExecutor() = default;

  [[nodiscard]] bool TryCompile() const override;

  [[nodiscard]] std::expected<void, std::runtime_error> Run(
      execution_tree::PassedExecutionData& data) override;
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_PLACEHOLDERJITEXECUTOR_HPP
