#include "CommandParser.hpp"

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"

#include "CommandFactory.hpp"
#include "IfParser.hpp"
#include "WhileParser.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> CommandParser::ParseSingleStatement(ParserContext& ctx,
                                                                             vm::execution_tree::Block& block) {
  if (ctx.IsEof()) {
    return std::unexpected(BytecodeParserError("Unexpected end of input"));
  }

  if (ctx.IsKeyword("if")) {
    IfParser parser;
    auto res = parser.Handle(ctx);

    if (!res) {
      return res;
    }

    ctx.current_block = &block;

    return {};
  }

  if (ctx.IsKeyword("while")) {
    WhileParser parser;

    auto res = parser.Handle(ctx);

    if (!res) {
      return res;
    }

    ctx.current_block = &block;

    return {};
  }

  auto token = ctx.Current();

  if (token->GetStringType() != "IDENT" && token->GetStringType() != "KEYWORD") {
    return std::unexpected(BytecodeParserError("Expected command name at line " +
                                                   std::to_string(token->GetPosition().GetLine()) + " column " +
                                                   std::to_string(token->GetPosition().GetColumn()),
                                               BytecodeParserErrorCode::kNotMatched));
  }

  std::string cmd_name = token->GetLexeme();

  ctx.Advance();

  CommandFactory factory;
  auto cmd_result = factory.CreateCommand(cmd_name, ctx);

  if (!cmd_result) {
    return std::unexpected(cmd_result.error());
  }

  block.AddStatement(std::move(cmd_result.value()));

  return {};
}

std::expected<void, BytecodeParserError> CommandParser::Handle(ParserContext& ctx) {
  return ParseSingleStatement(ctx, *ctx.current_block);
}

} // namespace ovum::bytecode::parser
