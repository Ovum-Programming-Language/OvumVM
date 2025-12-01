#ifndef EXECUTOR_PLACEHOLDERJITEXECUTORFACTORY_HPP
#define EXECUTOR_PLACEHOLDERJITEXECUTORFACTORY_HPP

#include <string>

#include "IJitExecutorFactory.hpp"
#include "PlaceholderJitExecutor.hpp"

namespace ovum::vm::executor {

class PlaceholderJitExecutorFactory : public IJitExecutorFactory {
public:
  [[nodiscard]] std::unique_ptr<IJitExecutor> Create(const std::string&) const override {
    return std::make_unique<PlaceholderJitExecutor>();
  }
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_PLACEHOLDERJITEXECUTORFACTORY_HPP
