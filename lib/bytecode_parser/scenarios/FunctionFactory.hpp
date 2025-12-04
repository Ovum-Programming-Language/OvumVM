#ifndef BYTECODE_PARSER_FUNCTIONFACTORY_HPP
#define BYTECODE_PARSER_FUNCTIONFACTORY_HPP

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/IFunctionExecutable.hpp"
#include "lib/execution_tree/JitCompilingFunction.hpp"
#include "lib/execution_tree/PureFunction.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/FunctionId.hpp"

namespace ovum::bytecode::parser {

class FunctionFactory {
public:
  FunctionFactory(std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory,
                  size_t jit_boundary);

  std::unique_ptr<vm::execution_tree::IFunctionExecutable> Create(const vm::runtime::FunctionId& id,
                                                                  size_t arity,
                                                                  std::unique_ptr<vm::execution_tree::Block> body,
                                                                  bool pure = false,
                                                                  std::vector<std::string> pure_argument_types = {},
                                                                  bool no_jit = false);

private:
  vm::execution_tree::Function MakeRegular(const vm::runtime::FunctionId& id,
                                           size_t arity,
                                           std::unique_ptr<vm::execution_tree::Block> body);

  template<vm::execution_tree::ExecutableFunction Base>
  vm::execution_tree::PureFunction<Base> WrapPure(Base&& base, std::vector<std::string> argument_types);

  template<vm::execution_tree::ExecutableFunction Base>
  std::unique_ptr<vm::execution_tree::JitCompilingFunction<Base>> TryWrapJit(Base&& base);

  std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory_;
  size_t jit_boundary_;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_FUNCTIONFACTORY_HPP
