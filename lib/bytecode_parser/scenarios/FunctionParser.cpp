#include "FunctionParser.hpp"

#include "lib/execution_tree/Block.hpp"

#include "CommandParser.hpp"
#include "FunctionFactory.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> FunctionParser::Handle(ParsingSessionPtr ctx) {
  std::vector<std::string> pure_types;
  bool is_pure = false;
  bool no_jit = false;

  if (ctx->IsKeyword("pure")) {
    ctx->Advance();

    std::expected<void, BytecodeParserError> e = ctx->ExpectPunct('(');

    if (!e) {
      return std::unexpected(e.error());
    }

    while (!ctx->IsPunct(')')) {
      std::expected<std::string, BytecodeParserError> type = ctx->ConsumeIdentifier();

      if (!type) {
        return std::unexpected(type.error());
      }

      pure_types.push_back(std::move(type.value()));

      if (ctx->IsPunct(',')) {
        ctx->Advance();
      }
    }

    e = ctx->ExpectPunct(')');

    if (!e) {
      return std::unexpected(e.error());
    }

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

  std::expected<void, BytecodeParserError> e = ctx->ExpectPunct(':');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::expected<int64_t, BytecodeParserError> arity_res = ctx->ConsumeIntLiteral();

  if (!arity_res) {
    return std::unexpected(arity_res.error());
  }

  size_t arity = static_cast<size_t>(arity_res.value());

  std::expected<std::string, BytecodeParserError> name_res = ctx->ConsumeIdentifier();

  if (!name_res) {
    return std::unexpected(name_res.error());
  }

  e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::Block> body = std::make_unique<vm::execution_tree::Block>();

  ctx->SetCurrentBlock(body.get());

  while (!ctx->IsPunct('}') && !ctx->IsEof()) {
    std::expected<void, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *body);

    if (!res) {
      return res;
    }
  }

  e = ctx->ExpectPunct('}');

  if (!e) {
    return std::unexpected(e.error());
  }

  ctx->SetCurrentBlock(nullptr);

  FunctionFactory factory(ctx->JitFactory(), ctx->JitBoundary());

  std::unique_ptr<vm::execution_tree::IFunctionExecutable> func =
      factory.Create(name_res.value(), arity, std::move(body), is_pure, std::move(pure_types), no_jit);

  std::expected<size_t, std::runtime_error> add_res = ctx->FuncRepo().Add(std::move(func));

  if (!add_res) {
    return std::unexpected(BytecodeParserError(std::string("Failed to add function: ") + add_res.error().what()));
  }

  return {};
}

} // namespace ovum::bytecode::parser
