#ifndef EXECUTOR_IJITEXECUTORFACTORY_HPP
#define EXECUTOR_IJITEXECUTORFACTORY_HPP

#include <memory>
#include <string>
#include <vector>

#include <tokens/Token.hpp>

#include "IJitExecutor.hpp"

namespace ovum::vm::executor {

class IJitExecutorFactory { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IJitExecutorFactory() = default;

  [[nodiscard]] virtual std::unique_ptr<IJitExecutor> Create(const std::string& function_name,
                                                             std::shared_ptr<std::vector<TokenPtr>> jit_body) const = 0;
};

} // namespace ovum::vm::executor

#endif // EXECUTOR_IJITEXECUTORFACTORY_HPP
