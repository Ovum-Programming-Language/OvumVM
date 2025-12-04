#include "CommandParser.hpp"

#include "CommandFactory.hpp"
#include "IfParser.hpp"
#include "WhileParser.hpp"

namespace ovum::bytecode::parser {

CommandParser::CommandParser(const ICommandFactory& factory) : factory_(factory) {
}

std::expected<bool, BytecodeParserError> CommandParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  return ParseSingleStatement(ctx, *ctx->CurrentBlock(), factory_);
}

std::expected<bool, BytecodeParserError> CommandParser::ParseSingleStatement(const std::shared_ptr<ParsingSession>& ctx,
                                                                             vm::execution_tree::Block& block,
                                                                             const ICommandFactory& factory) {
  if (ctx->IsEof()) {
    return std::unexpected(BytecodeParserError("Unexpected end of input"));
  }

  if (ctx->IsKeyword("if")) {
    IfParser parser(factory);
    std::expected<bool, BytecodeParserError> res = parser.Handle(ctx);

    if (!res) {
      return res;
    }

    ctx->SetCurrentBlock(&block);

    return res;
  }

  if (ctx->IsKeyword("while")) {
    WhileParser parser(factory);
    std::expected<bool, BytecodeParserError> res = parser.Handle(ctx);

    if (!res) {
      return res;
    }

    ctx->SetCurrentBlock(&block);

    return res;
  }

  TokenPtr token = ctx->Current();

  if (token->GetStringType() != "IDENT" && token->GetStringType() != "KEYWORD") {
    return false;
  }

  std::string cmd_name = token->GetLexeme();

  ctx->Advance();

  std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError> cmd_result =
      factory.CreateCommand(cmd_name, ctx);

  if (!cmd_result) {
    return std::unexpected(cmd_result.error());
  }

  block.AddStatement(std::move(cmd_result.value()));

  return true;
}

ICommandFactory& CommandParser::DefaultFactory() {
  static CommandFactory instance;
  return instance;
}

std::expected<bool, BytecodeParserError> CommandParser::ParseSingleStatement(const std::shared_ptr<ParsingSession>& ctx,
                                                                             vm::execution_tree::Block& block) {
  CommandFactory cmd_factory;
  return ParseSingleStatement(ctx, block, cmd_factory);
}

} // namespace ovum::bytecode::parser
