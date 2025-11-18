#include "CommandParser.hpp"
#include "WhileParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/WhileExecution.hpp"

namespace ovum::bytecode::parser {

bool WhileParser::Handle(ParserContext& ctx) {
  if (!ctx.IsKeyword("while"))
    return false;
  ctx.Advance();

  if (auto e = ctx.ExpectPunct('{'); !e)
    throw e.error();
  auto condition = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = condition.get();
  while (!ctx.IsPunct('}')) {
    if (!CommandParser::ParseSingleStatement(ctx, *condition))
      throw BytecodeParserError("Invalid statement in while condition");
  }
  if (auto e = ctx.ExpectPunct('}'); !e)
    throw e.error();

  if (auto e = ctx.ExpectKeyword("then"); !e)
    throw e.error();

  if (auto e = ctx.ExpectPunct('{'); !e)
    throw e.error();
  auto body = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = body.get();
  while (!ctx.IsPunct('}')) {
    if (!CommandParser::ParseSingleStatement(ctx, *body))
      throw BytecodeParserError("Invalid statement in while body");
  }
  if (auto e = ctx.ExpectPunct('}'); !e)
    throw e.error();

  auto while_node = std::make_unique<vm::execution_tree::WhileExecution>(std::move(condition), std::move(body));

  ctx.current_block->AddStatement(std::move(while_node));
  return true;
}

} // namespace ovum::bytecode::parser
