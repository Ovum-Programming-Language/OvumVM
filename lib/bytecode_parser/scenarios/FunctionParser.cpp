#include "CommandParser.hpp"
#include "FunctionFactory.hpp"
#include "FunctionParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/execution_tree/Block.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> FunctionParser::Handle(ParsingSessionPtr ctx) {
  std::vector<std::string> pure_types;
  bool is_pure = false;
  bool no_jit = false;

  if (ctx->IsKeyword("pure")) {
    ctx->Advance();
    if (auto e = ctx->ExpectPunct('('); !e)
      return std::unexpected(e.error());

    while (!ctx->IsPunct(')')) {
      auto type = ctx->ConsumeIdentifier();
      if (!type)
        return std::unexpected(type.error());
      pure_types.push_back(std::move(type.value()));
      if (ctx->IsPunct(','))
        ctx->Advance();
    }
    if (auto e = ctx->ExpectPunct(')'); !e)
      return std::unexpected(e.error());
    is_pure = true;
  }

  if (ctx->IsKeyword("no-jit")) {
    ctx->Advance();
    no_jit = true;
  }

  if (!ctx->IsKeyword("function")) {
    return std::unexpected(BytecodeParserError("Expected 'function'", BytecodeParserErrorCode::kNotMatched));
  }

  ctx->Advance();
  if (auto e = ctx->ExpectPunct(':'); !e)
    return std::unexpected(e.error());

  auto arity_res = ctx->ConsumeIntLiteral();
  if (!arity_res)
    return std::unexpected(arity_res.error());
  size_t arity = static_cast<size_t>(arity_res.value());

  auto name_res = ctx->ConsumeIdentifier();
  if (!name_res)
    return std::unexpected(name_res.error());

  if (auto e = ctx->ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto body = std::make_unique<vm::execution_tree::Block>();
  ctx->SetCurrentBlock(body.get());

  while (!ctx->IsPunct('}') && !ctx->IsEof()) {
    auto res = CommandParser::ParseSingleStatement(ctx, *body);
    if (!res)
      return res;
  }

  if (auto e = ctx->ExpectPunct('}'); !e)
    return std::unexpected(e.error());
  ctx->SetCurrentBlock(nullptr);

  FunctionFactory factory(ctx->JitFactory(), ctx->JitBoundary());

  auto func = factory.Create(name_res.value(), arity, std::move(body), is_pure, std::move(pure_types), no_jit);

  auto add_res = ctx->FuncRepo().Add(std::move(func));
  if (!add_res) {
    return std::unexpected(BytecodeParserError(std::string("Failed to add function: ") + add_res.error().what()));
  }

  return {};
}

} // namespace ovum::bytecode::parser
