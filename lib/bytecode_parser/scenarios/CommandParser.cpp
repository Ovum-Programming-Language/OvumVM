#include "CommandParser.hpp"

#include "IfParser.hpp"
#include "WhileParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/command_factory.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> CommandParser::ParseSingleStatement(ParserContext& ctx,
                                                                             vm::execution_tree::Block& block) {
  if (ctx.IsEof())
    return std::unexpected(BytecodeParserError("Unexpected end of input"));

  if (ctx.IsKeyword("if")) {
    IfParser parser;
    auto res = parser.Handle(ctx);

    if (!res)
      return res;

    ctx.current_block = &block;

    return {};
  }

  if (ctx.IsKeyword("while")) {
    WhileParser parser;

    auto res = parser.Handle(ctx);

    if (!res)
      return res;

    ctx.current_block = &block;

    return {};
  }

  auto token = ctx.Current();

  if (token->GetStringType() != "IDENT" && token->GetStringType() != "KEYWORD") {
    return std::unexpected(BytecodeParserError("Expected command name at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  std::string cmd = token->GetLexeme();
  ctx.Advance();

  static const std::unordered_set<std::string> kStringCommands = {"PushString", "PushChar"};

  static const std::unordered_set<std::string> kIntegerCommands = {
      "PushInt", "PushByte", "Rotate", "LoadLocal", "SetLocal", "LoadStatic", "SetStatic", "GetField", "SetField"};

  static const std::unordered_set<std::string> kFloatCommands = {"PushFloat"};

  static const std::unordered_set<std::string> kBooleanCommands = {"PushBool"};

  static const std::unordered_set<std::string> kIdentCommands = {
      "NewArray", "Call", "CallVirtual", "CallConstructor", "GetVTable", "SetVTable", "SafeCall", "IsType", "SizeOf"};

  if (kStringCommands.contains(cmd)) {
    auto value = ctx.ConsumeStringLiteral();

    if (!value)
      return std::unexpected(value.error());

    auto exec = ovum::vm::execution_tree::CreateStringCommandByName(cmd, value.value());

    if (!exec)
      return std::unexpected(BytecodeParserError("Failed to create string command: " + cmd));

    block.AddStatement(std::move(exec.value()));
  } else if (kIntegerCommands.contains(cmd)) {
    auto value = ctx.ConsumeIntLiteral();

    if (!value)
      return std::unexpected(value.error());

    auto exec = ovum::vm::execution_tree::CreateIntegerCommandByName(cmd, value.value());

    if (!exec)
      return std::unexpected(BytecodeParserError("Failed to create integer command: " + cmd));

    block.AddStatement(std::move(exec.value()));
  } else if (kFloatCommands.contains(cmd)) {
    auto value = ctx.ConsumeFloatLiteral();

    if (!value)
      return std::unexpected(value.error());

    auto exec = ovum::vm::execution_tree::CreateFloatCommandByName(cmd, value.value());

    if (!exec)
      return std::unexpected(BytecodeParserError("Failed to create float command: " + cmd));

    block.AddStatement(std::move(exec.value()));
  } else if (kBooleanCommands.contains(cmd)) {
    auto value = ctx.ConsumeBoolLiteral();

    if (!value)
      return std::unexpected(value.error());

    auto exec = ovum::vm::execution_tree::CreateBooleanCommandByName(cmd, value.value());

    if (!exec)
      return std::unexpected(BytecodeParserError("Failed to create boolean command: " + cmd));

    block.AddStatement(std::move(exec.value()));
  } else if (kIdentCommands.contains(cmd)) {
    auto func_name = ctx.ConsumeIdentifier();

    if (!func_name)
      return std::unexpected(func_name.error());

    auto exec = ovum::vm::execution_tree::CreateStringCommandByName(cmd, func_name.value());

    if (!exec)
      return std::unexpected(BytecodeParserError("Failed to create call command: " + cmd));

    block.AddStatement(std::move(exec.value()));
  } else {
    auto exec = ovum::vm::execution_tree::CreateSimpleCommandByName(cmd);

    if (!exec)
      return std::unexpected(BytecodeParserError("Unknown command: " + cmd));

    block.AddStatement(std::move(exec.value()));
  }

  return {};
}

std::expected<void, BytecodeParserError> CommandParser::Handle(ParserContext& ctx) {
  return ParseSingleStatement(ctx, *ctx.current_block);
}

} // namespace ovum::bytecode::parser
