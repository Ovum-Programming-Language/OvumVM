#include "CommandParser.hpp"
#include "WhileParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/WhileExecution.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> WhileParser::Handle(ParserContext& ctx) {
  if (!ctx.IsKeyword("while"))
    return std::unexpected(BytecodeParserError("Expected 'while'"));

  ctx.Advance();

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());
  auto condition = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = condition.get();
  while (!ctx.IsPunct('}')) {
    auto res = CommandParser::ParseSingleStatement(ctx, *condition);
    if (!res)
      return res;
  }
  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  if (auto e = ctx.ExpectKeyword("then"); !e)
    return std::unexpected(e.error());

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());
  auto body = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = body.get();
  while (!ctx.IsPunct('}')) {
    auto res = CommandParser::ParseSingleStatement(ctx, *body);
    if (!res)
      return res;
  }
  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  auto while_node = std::make_unique<vm::execution_tree::WhileExecution>(std::move(condition), std::move(body));

  ctx.current_block->AddStatement(std::move(while_node));
  return {};
}

} // namespace ovum::bytecode::parser
