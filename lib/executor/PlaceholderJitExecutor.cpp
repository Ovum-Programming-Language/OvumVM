#include "PlaceholderJitExecutor.hpp"

namespace ovum::vm::executor {

bool PlaceholderJitExecutor::TryCompile() {
  return false;
}

std::expected<void, std::runtime_error> PlaceholderJitExecutor::Run(execution_tree::PassedExecutionData& /* data */) {
  return std::unexpected(std::runtime_error("PlaceholderJitExecutor::Run: not implemented"));
}

} // namespace ovum::vm::executor
