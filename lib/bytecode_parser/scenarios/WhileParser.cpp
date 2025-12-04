#include "WhileParser.hpp"

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/WhileExecution.hpp"

#include "CommandFactory.hpp"
#include "CommandParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

WhileParser::WhileParser(const ICommandFactory& factory) : factory_(factory) {
}

std::expected<bool, BytecodeParserError> WhileParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  if (!ctx->IsKeyword("while")) {
    return false;
  }

  ctx->Advance();

  std::expected<void, BytecodeParserError> e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::Block> condition = std::make_unique<vm::execution_tree::Block>();

  ctx->SetCurrentBlock(condition.get());

  while (!ctx->IsPunct('}')) {
    std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *condition, factory_);

    if (!res) {
      return res;
    } else if (!res.value()) {
      return std::unexpected(
          BytecodeParserError("Command expected at line" + std::to_string(ctx->Current()->GetPosition().GetLine())));
    }
  }

  e = ctx->ExpectPunct('}');

  if (!e) {
    return std::unexpected(e.error());
  }

  e = ctx->ExpectKeyword("then");

  if (!e) {
    return std::unexpected(e.error());
  }

  e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::Block> body = std::make_unique<vm::execution_tree::Block>();

  ctx->SetCurrentBlock(body.get());

  while (!ctx->IsPunct('}')) {
    std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *body, factory_);

    if (!res) {
      return res;
    } else if (!res.value()) {
      return std::unexpected(
          BytecodeParserError("Command expected at line" + std::to_string(ctx->Current()->GetPosition().GetLine())));
    }
  }

  e = ctx->ExpectPunct('}');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::WhileExecution> while_node =
      std::make_unique<vm::execution_tree::WhileExecution>(std::move(condition), std::move(body));

  ctx->CurrentBlock()->AddStatement(std::move(while_node));

  return true;
}

} // namespace ovum::bytecode::parser
