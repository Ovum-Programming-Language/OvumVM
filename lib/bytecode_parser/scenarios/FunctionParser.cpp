#include "FunctionParser.hpp"

#include "CommandParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/JitCompilingFunction.hpp"
#include "lib/execution_tree/PureFunction.hpp"
#include "lib/executor/PlaceholderJitExecutor.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> FunctionParser::Handle(ParserContext& ctx) {
  std::vector<std::string> pure_types;
  bool is_pure = false;
  bool no_jit = false;

  if (ctx.IsKeyword("pure")) {
    ctx.Advance();
    if (auto e = ctx.ExpectPunct('('); !e)
      return std::unexpected(e.error());
    while (!ctx.IsPunct(')')) {
      auto type = ctx.ConsumeIdentifier();
      if (!type)
        return std::unexpected(type.error());
      pure_types.push_back(type.value());
      if (ctx.IsPunct(','))
        ctx.Advance();
    }
    if (auto e = ctx.ExpectPunct(')'); !e)
      return std::unexpected(e.error());
    is_pure = true;
  }

  if (ctx.IsKeyword("no-jit")) {
    ctx.Advance();
    no_jit = true;
  }

  if (!ctx.IsKeyword("function"))
    return std::unexpected(BytecodeParserError("Expected 'function'"));

  ctx.Advance();

  if (auto e = ctx.ExpectPunct(':'); !e)
    return std::unexpected(e.error());

  auto arity_res = ctx.ConsumeIntLiteral();
  if (!arity_res)
    return std::unexpected(arity_res.error());
  size_t arity = static_cast<size_t>(arity_res.value());

  auto name_res = ctx.ConsumeIdentifier();
  if (!name_res)
    return std::unexpected(name_res.error());
  std::string name = name_res.value();

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto body = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = body.get();

  while (!ctx.IsPunct('}') && !ctx.IsEof()) {
    auto res = CommandParser::ParseSingleStatement(ctx, *body);
    if (!res)
      return res;
  }

  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());
  ctx.current_block = nullptr;

  auto func = std::make_unique<vm::execution_tree::Function>(name, arity, std::move(body));

  std::unique_ptr<vm::execution_tree::IFunctionExecutable> final_func;

  if (is_pure && !pure_types.empty()) {
    auto pure_func = std::make_unique<vm::execution_tree::PureFunction<vm::execution_tree::Function>>(
        std::move(*func), std::move(pure_types));

    if (!no_jit) {
      auto jit_executor = std::make_unique<vm::executor::PlaceholderJitExecutor>();
      final_func = std::make_unique<
          vm::execution_tree::JitCompilingFunction<vm::execution_tree::PureFunction<vm::execution_tree::Function>>>(
          std::move(jit_executor), std::move(*pure_func), 100);
    } else {
      final_func = std::move(pure_func);
    }
  } else {
    if (!no_jit) {
      auto jit_executor = std::make_unique<vm::executor::PlaceholderJitExecutor>();
      final_func = std::make_unique<vm::execution_tree::JitCompilingFunction<vm::execution_tree::Function>>(
          std::move(jit_executor), std::move(*func), 100);
    } else {
      final_func = std::move(func);
    }
  }

  auto add_res = ctx.func_repo.Add(std::move(final_func));
  if (!add_res) {
    return std::unexpected(BytecodeParserError(std::string("Failed to add function: ") + add_res.error().what()));
  }

  return {};
}

} // namespace ovum::bytecode::parser
