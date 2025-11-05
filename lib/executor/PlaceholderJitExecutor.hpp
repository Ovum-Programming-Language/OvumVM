#ifndef EXECUTOR_PLACEHOLDERJITEXECUTOR_HPP
#define EXECUTOR_PLACEHOLDERJITEXECUTOR_HPP

#include "IJitExecutor.hpp"

namespace ovum::vm::executor {

class PlaceholderJitExecutor : public IJitExecutor {
public:
  PlaceholderJitExecutor() = default;

  [[nodiscard]] bool TryCompile() const override;

  [[nodiscard]] std::expected<void, std::runtime_error> Run(
      std::stack<std::variant<int64_t, double, bool, char, uint8_t, void*>>& stack) override;
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_PLACEHOLDERJITEXECUTOR_HPP
