#ifndef BYTECODE_PARSER_FUNCTIONFACTORY_HPP
#define BYTECODE_PARSER_FUNCTIONFACTORY_HPP

#include <expected>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/ExecutionConcepts.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/JitCompilingFunction.hpp"
#include "lib/execution_tree/PureFunction.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/FunctionId.hpp"

namespace ovum::bytecode::parser {

class FunctionFactory {
public:
  FunctionFactory(vm::executor::IJitExecutorFactory* jit_factory, size_t jit_boundary);

  vm::execution_tree::Function CreateRegular(const vm::runtime::FunctionId& id,
                                             size_t arity,
                                             std::unique_ptr<vm::execution_tree::Block> body);

  template<vm::execution_tree::ExecutableFunction BaseFunction>
  vm::execution_tree::PureFunction<BaseFunction> MakePure(BaseFunction&& base_function,
                                                          std::vector<std::string> argument_type_names);

  template<vm::execution_tree::ExecutableFunction BaseFunction>
  vm::execution_tree::JitCompilingFunction<BaseFunction> MakeJit(BaseFunction&& base_function);

private:
  vm::executor::IJitExecutorFactory* jit_factory_;
  size_t jit_boundary_;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_FUNCTIONFACTORY_HPP
