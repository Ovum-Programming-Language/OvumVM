#include "FunctionFactory.hpp"


namespace ovum::bytecode::parser {

FunctionFactory::FunctionFactory(vm::executor::IJitExecutorFactory* jit_factory,
                                 size_t jit_boundary)
    : jit_factory_(jit_factory), jit_boundary_(jit_boundary) {
}

std::unique_ptr<vm::execution_tree::Function> FunctionFactory::CreateRegular(
    const vm::runtime::FunctionId& id,
    size_t arity,
    std::unique_ptr<vm::execution_tree::Block> body) {

  return std::make_unique<vm::execution_tree::Function>(id, arity, std::move(body));
}

template <vm::execution_tree::ExecutableFunction BaseFunction>
std::unique_ptr<vm::execution_tree::PureFunction<BaseFunction>> FunctionFactory::MakePure(
    std::unique_ptr<BaseFunction> base_function,
    const std::vector<std::string>& argument_type_names) {

  return std::make_unique<vm::execution_tree::PureFunction<BaseFunction>>(
      std::move(*base_function), std::move(argument_type_names));
}

template <vm::execution_tree::ExecutableFunction BaseFunction>
std::unique_ptr<vm::execution_tree::JitCompilingFunction<BaseFunction>> FunctionFactory::TryMakeJit(
    std::unique_ptr<BaseFunction> base_function) {

  auto jit_executor = jit_factory_->Create();

  return std::make_unique<vm::execution_tree::JitCompilingFunction<BaseFunction>>(
      std::move(jit_executor), std::move(*base_function), jit_boundary_);
}

template std::unique_ptr<vm::execution_tree::PureFunction<vm::execution_tree::Function>> FunctionFactory::MakePure<vm::execution_tree::Function>(
    std::unique_ptr<vm::execution_tree::Function> base_function,
    const std::vector<std::string>& argument_type_names);

template std::unique_ptr<vm::execution_tree::JitCompilingFunction<vm::execution_tree::Function>> FunctionFactory::TryMakeJit<vm::execution_tree::Function>(
    std::unique_ptr<vm::execution_tree::Function> base_function);

template std::unique_ptr<vm::execution_tree::JitCompilingFunction<vm::execution_tree::PureFunction<vm::execution_tree::Function>>> FunctionFactory::TryMakeJit<vm::execution_tree::PureFunction<vm::execution_tree::Function>>(
    std::unique_ptr<vm::execution_tree::PureFunction<vm::execution_tree::Function>> base_function);

} // namespace ovum::bytecode::parser
