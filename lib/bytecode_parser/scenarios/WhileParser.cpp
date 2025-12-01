#include "CommandParser.hpp"
#include "WhileParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/WhileExecution.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> WhileParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  if (!ctx->IsKeyword("while")) {
    return std::unexpected(BytecodeParserError("Expected 'while'", BytecodeParserErrorCode::kNotMatched));
  }

  ctx->Advance();

  if (auto e = ctx->ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto condition = std::make_unique<vm::execution_tree::Block>();
  ctx->SetCurrentBlock(condition.get());

  while (!ctx->IsPunct('}')) {
    auto res = CommandParser::ParseSingleStatement(ctx, *condition);
    if (!res)
      return res;
  }

  if (auto e = ctx->ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  if (auto e = ctx->ExpectKeyword("then"); !e)
    return std::unexpected(e.error());

  if (auto e = ctx->ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto body = std::make_unique<vm::execution_tree::Block>();
  ctx->SetCurrentBlock(body.get());

  while (!ctx->IsPunct('}')) {
    auto res = CommandParser::ParseSingleStatement(ctx, *body);
    if (!res)
      return res;
  }

  if (auto e = ctx->ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  auto while_node = std::make_unique<vm::execution_tree::WhileExecution>(std::move(condition), std::move(body));
  ctx->CurrentBlock()->AddStatement(std::move(while_node));

  return {};
}

} // namespace ovum::bytecode::parser
