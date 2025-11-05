#include "PlaceholderJitExecutor.hpp"

namespace ovum::vm::executor {

bool PlaceholderJitExecutor::TryCompile() const {
  return false;
}

std::expected<void, std::runtime_error> PlaceholderJitExecutor::Run(
    std::stack<std::variant<int64_t, double, bool, char, uint8_t, void*>>& /* stack */) {
  return std::unexpected(std::runtime_error("PlaceholderJitExecutor::Run: not implemented"));
}

} // namespace ovum::vm::executor
