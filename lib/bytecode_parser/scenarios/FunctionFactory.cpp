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
vm::execution_tree::PureFunction<Base> FunctionFactory::WrapPure(Base&& base,
                                                                 std::vector<std::string>&& argument_types) {
  return vm::execution_tree::PureFunction<Base>(std::forward<Base>(base), std::move(argument_types));
}

template<vm::execution_tree::ExecutableFunction Base>
std::expected<vm::execution_tree::JitCompilingFunction<Base>, std::runtime_error> FunctionFactory::WrapJit(
    Base&& base, std::shared_ptr<std::vector<TokenPtr>> jit_body) {
  if (!jit_factory_.has_value()) {
    return std::unexpected(std::runtime_error("Jit factory not set"));
  }

  return {vm::execution_tree::JitCompilingFunction<Base>(
      jit_factory_->get().Create(base.GetId(), std::move(jit_body)), std::forward<Base>(base), jit_boundary_)};
}

std::unique_ptr<vm::execution_tree::IFunctionExecutable> FunctionFactory::Create(
    const vm::runtime::FunctionId& id,
    size_t arity,
    std::unique_ptr<vm::execution_tree::Block> body,
    bool pure,
    std::vector<std::string> pure_argument_types,
    bool no_jit,
    std::shared_ptr<std::vector<TokenPtr>> jit_body) {
  RegularFunction regular = MakeRegular(id, arity, std::move(body));

  if (!pure || pure_argument_types.empty()) {
    if (no_jit || !jit_factory_.has_value()) {
      return std::make_unique<RegularFunction>(std::move(regular));
    }

    std::expected<JitFunction, std::runtime_error> jit_func = WrapJit(std::move(regular), std::move(jit_body));

    if (!jit_func) {
      return nullptr;
    }

    return std::make_unique<JitFunction>(std::move(jit_func.value()));
  }

  if (no_jit || !jit_factory_.has_value()) {
    return std::make_unique<PureFunction>(WrapPure(std::move(regular), std::move(pure_argument_types)));
  }

  std::expected<JitFunction, std::runtime_error> jit_func = WrapJit(std::move(regular), std::move(jit_body));

  if (!jit_func) {
    return nullptr;
  }

  return std::make_unique<PureJitFunction>(
      std::move(WrapPure(std::move(jit_func.value()), std::move(pure_argument_types))));
}

} // namespace ovum::bytecode::parser
