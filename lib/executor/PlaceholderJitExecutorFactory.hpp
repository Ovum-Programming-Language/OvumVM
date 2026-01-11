#ifndef EXECUTOR_PLACEHOLDERJITEXECUTORFACTORY_HPP
#define EXECUTOR_PLACEHOLDERJITEXECUTORFACTORY_HPP

#include <string>
#include <vector>

#include <tokens/Token.hpp>

#include "IJitExecutorFactory.hpp"

namespace ovum::vm::executor {

class PlaceholderJitExecutorFactory : public IJitExecutorFactory {
public:
  [[nodiscard]] std::unique_ptr<IJitExecutor> Create(const std::string&,
                                                     std::shared_ptr<std::vector<TokenPtr>>) const override;
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_PLACEHOLDERJITEXECUTORFACTORY_HPP
