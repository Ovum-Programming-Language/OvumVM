#include "PlaceholderJitExecutorFactory.hpp"

#include "PlaceholderJitExecutor.hpp"

namespace ovum::vm::executor {

std::unique_ptr<IJitExecutor> PlaceholderJitExecutorFactory::Create(
    const std::string&,
    std::unique_ptr<std::vector<TokenPtr>>
  ) const {
  return std::make_unique<PlaceholderJitExecutor>();
}

} // namespace ovum::vm::executor
