#include "FunctionFactory.hpp"

namespace ovum::bytecode::parser {

FunctionFactory::FunctionFactory(std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory,
                                 size_t jit_boundary) : jit_factory_(jit_factory), jit_boundary_(jit_boundary) {
}

vm::execution_tree::Function FunctionFactory::MakeRegular(const vm::runtime::FunctionId& id,
                                                          size_t arity,
                                                          std::unique_ptr<vm::execution_tree::Block> body) {
  return {id, arity, std::move(body)};
}

template<vm::execution_tree::ExecutableFunction Base>
vm::execution_tree::PureFunction<Base> FunctionFactory::WrapPure(Base&& base, std::vector<std::string> argument_types) {
  return vm::execution_tree::PureFunction<Base>(std::forward<Base>(base), std::move(argument_types));
}

template<vm::execution_tree::ExecutableFunction Base>
std::unique_ptr<vm::execution_tree::JitCompilingFunction<Base>> FunctionFactory::TryWrapJit(Base&& base) {
  if (!jit_factory_.has_value()) {
    return nullptr;
  }

  std::unique_ptr<vm::executor::IJitExecutor> executor = jit_factory_->get().Create(base.GetId());

  return std::make_unique<vm::execution_tree::JitCompilingFunction<Base>>(
      std::move(executor), std::forward<Base>(base), jit_boundary_);
}

std::unique_ptr<vm::execution_tree::IFunctionExecutable> FunctionFactory::Create(
    const vm::runtime::FunctionId& id,
    size_t arity,
    std::unique_ptr<vm::execution_tree::Block> body,
    bool pure,
    std::vector<std::string> pure_argument_types,
    bool no_jit) {
  vm::execution_tree::Function regular = MakeRegular(id, arity, std::move(body));

  if (!pure || pure_argument_types.empty()) {
    if (no_jit || !jit_factory_.has_value()) {
      return std::make_unique<vm::execution_tree::Function>(std::move(regular));
    }

    std::unique_ptr<vm::execution_tree::JitCompilingFunction<vm::execution_tree::Function>> jit =
        TryWrapJit(std::move(regular));

    if (jit) {
      return std::make_unique<vm::execution_tree::JitCompilingFunction<vm::execution_tree::Function>>(std::move(*jit));
    }

    return std::make_unique<vm::execution_tree::Function>(std::move(regular));
  }

  vm::execution_tree::PureFunction<vm::execution_tree::Function> pure_func =
      WrapPure(std::move(regular), std::move(pure_argument_types));

  if (no_jit || !jit_factory_.has_value()) {
    return std::make_unique<vm::execution_tree::PureFunction<vm::execution_tree::Function>>(std::move(pure_func));
  }

  std::unique_ptr<
      vm::execution_tree::JitCompilingFunction<vm::execution_tree::PureFunction<vm::execution_tree::Function>>>
      jit = TryWrapJit(std::move(pure_func));

  if (jit) {
    return std::make_unique<
        vm::execution_tree::JitCompilingFunction<vm::execution_tree::PureFunction<vm::execution_tree::Function>>>(
        std::move(*jit));
  }

  return std::make_unique<vm::execution_tree::PureFunction<vm::execution_tree::Function>>(std::move(pure_func));
}

template vm::execution_tree::PureFunction<vm::execution_tree::Function>
FunctionFactory::WrapPure<vm::execution_tree::Function>(vm::execution_tree::Function&& base,
                                                        std::vector<std::string> argument_types);

template std::unique_ptr<vm::execution_tree::JitCompilingFunction<vm::execution_tree::Function>>
FunctionFactory::TryWrapJit<vm::execution_tree::Function>(vm::execution_tree::Function&& base);

template std::unique_ptr<
    vm::execution_tree::JitCompilingFunction<vm::execution_tree::PureFunction<vm::execution_tree::Function>>>
FunctionFactory::TryWrapJit<vm::execution_tree::PureFunction<vm::execution_tree::Function>>(
    vm::execution_tree::PureFunction<vm::execution_tree::Function>&& base);

} // namespace ovum::bytecode::parser
