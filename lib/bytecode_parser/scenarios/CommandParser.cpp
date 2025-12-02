#include "CommandParser.hpp"

#include "CommandFactory.hpp"
#include "IfParser.hpp"
#include "WhileParser.hpp"

namespace ovum::bytecode::parser {

CommandParser::CommandParser(std::unique_ptr<ICommandFactory> factory) :
    factory_(factory ? std::move(factory) : std::make_unique<CommandFactory>()) {
}

std::expected<void, BytecodeParserError> CommandParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  return ParseSingleStatement(ctx, *ctx->CurrentBlock(), *factory_);
}

std::expected<void, BytecodeParserError> CommandParser::ParseSingleStatement(const std::shared_ptr<ParsingSession>& ctx,
                                                                             vm::execution_tree::Block& block,
                                                                             ICommandFactory& factory) {
  if (ctx->IsEof()) {
    return std::unexpected(BytecodeParserError("Unexpected end of input"));
  }

  if (ctx->IsKeyword("if")) {
    IfParser parser;
    std::expected<void, BytecodeParserError> res = parser.Handle(ctx);

    if (!res) {
      return res;
    }

    ctx->SetCurrentBlock(&block);

    return {};
  }

  if (ctx->IsKeyword("while")) {
    WhileParser parser;
    std::expected<void, BytecodeParserError> res = parser.Handle(ctx);

    if (!res) {
      return res;
    }

    ctx->SetCurrentBlock(&block);

    return {};
  }

  TokenPtr token = ctx->Current();

  if (token->GetStringType() != "IDENT" && token->GetStringType() != "KEYWORD") {
    return std::unexpected(BytecodeParserError("Expected command name at line " +
                                                   std::to_string(token->GetPosition().GetLine()) + " column " +
                                                   std::to_string(token->GetPosition().GetColumn()),
                                               BytecodeParserErrorCode::kNotMatched));
  }

  std::string cmd_name = token->GetLexeme();

  ctx->Advance();

  std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError> cmd_result =
      factory.CreateCommand(cmd_name, ctx);

  if (!cmd_result) {
    return std::unexpected(cmd_result.error());
  }

  block.AddStatement(std::move(cmd_result.value()));

  return {};
}

ICommandFactory& CommandParser::DefaultFactory() {
  static CommandFactory instance;
  return instance;
}

std::expected<void, BytecodeParserError> CommandParser::ParseSingleStatement(std::shared_ptr<ParsingSession> ctx,
                                                                             vm::execution_tree::Block& block) {
  return ParseSingleStatement(ctx, block, DefaultFactory());
}

} // namespace ovum::bytecode::parser
