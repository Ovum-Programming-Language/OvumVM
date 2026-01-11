#ifndef EXECUTOR_IJITEXECUTOR_HPP
#define EXECUTOR_IJITEXECUTOR_HPP

#include <cstdint>
#include <expected>
#include <stack>
#include <stdexcept>
#include <variant>

#include "lib/execution_tree/PassedExecutionData.hpp"

namespace ovum::vm::executor {

class IJitExecutor { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IJitExecutor() = default;

  [[nodiscard]] virtual bool TryCompile() const = 0;

  [[nodiscard]] virtual std::expected<void, std::runtime_error> Run(execution_tree::PassedExecutionData& data) = 0;
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_IJITEXECUTOR_HPP
