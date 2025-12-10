#ifndef EXECUTOR_EXECUTOR_HPP
#define EXECUTOR_EXECUTOR_HPP

#include <expected>
#include <memory>
#include <stdexcept>
#include <string>

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"

namespace ovum::vm::executor {

class Executor {
public:
  explicit Executor(execution_tree::PassedExecutionData& execution_data);

  [[nodiscard]] std::expected<execution_tree::ExecutionResult, std::runtime_error> RunProgram(
      const std::unique_ptr<execution_tree::Block>& init_static);

private:
  static const std::string kMainFunctionName;
  execution_tree::PassedExecutionData& execution_data_;
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_EXECUTOR_HPP
