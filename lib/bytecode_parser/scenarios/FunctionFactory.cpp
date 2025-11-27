#include "FunctionFactory.hpp"

namespace ovum::bytecode::parser {

FunctionFactory::FunctionFactory(vm::executor::IJitExecutorFactory* jit_factory, size_t jit_boundary) :
    jit_factory_(jit_factory), jit_boundary_(jit_boundary) {
}

vm::execution_tree::Function FunctionFactory::CreateRegular(const vm::runtime::FunctionId& id,
                                                            size_t arity,
                                                            std::unique_ptr<vm::execution_tree::Block> body) {
  return {id, arity, std::move(body)};
}

template<vm::execution_tree::ExecutableFunction BaseFunction>
vm::execution_tree::PureFunction<BaseFunction> FunctionFactory::MakePure(BaseFunction&& base_function,
                                                                         std::vector<std::string> argument_type_names) {
  return vm::execution_tree::PureFunction<BaseFunction>(std::forward<BaseFunction>(base_function),
                                                        std::move(argument_type_names));
}

template<vm::execution_tree::ExecutableFunction BaseFunction>
vm::execution_tree::JitCompilingFunction<BaseFunction> FunctionFactory::MakeJit(BaseFunction&& base_function) {
  auto jit_executor = jit_factory_->Create();

  return vm::execution_tree::JitCompilingFunction<BaseFunction>(
      std::move(jit_executor), std::forward<BaseFunction>(base_function), jit_boundary_);
}

template vm::execution_tree::PureFunction<vm::execution_tree::Function>
FunctionFactory::MakePure<vm::execution_tree::Function>(vm::execution_tree::Function&& base_function,
                                                        std::vector<std::string> argument_type_names);

template vm::execution_tree::JitCompilingFunction<vm::execution_tree::Function>
FunctionFactory::MakeJit<vm::execution_tree::Function>(vm::execution_tree::Function&& base_function);

template vm::execution_tree::JitCompilingFunction<vm::execution_tree::PureFunction<vm::execution_tree::Function>>
FunctionFactory::MakeJit<vm::execution_tree::PureFunction<vm::execution_tree::Function>>(
    vm::execution_tree::PureFunction<vm::execution_tree::Function>&& base_function);

} // namespace ovum::bytecode::parser
