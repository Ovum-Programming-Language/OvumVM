#include "CommandParser.hpp"

#include "IfParser.hpp"
#include "WhileParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/command_factory.hpp"

namespace ovum::bytecode::parser {

bool CommandParser::ParseSingleStatement(ParserContext& ctx, vm::execution_tree::Block& block) {
  if (ctx.IsEof())
    return false;

  if (ctx.IsKeyword("if")) {
    IfParser parser;
    return parser.Handle(ctx) && (ctx.current_block = &block, true);
  }

  if (ctx.IsKeyword("while")) {
    WhileParser parser;
    return parser.Handle(ctx) && (ctx.current_block = &block, true);
  }

  auto token = ctx.Current();
  if (token->GetStringType() != "IDENT" && token->GetStringType() != "KEYWORD") {
    throw BytecodeParserError("Expected command name at line " + std::to_string(token->GetPosition().GetLine()) +
                              " column " + std::to_string(token->GetPosition().GetColumn()));
  }

  std::string cmd = token->GetLexeme();
  ctx.Advance();

  static const std::unordered_set<std::string> kStringCommands = {
      "PushString", "FormatDateTime", "GetEnvironmentVariable", "SetEnvironmentVariable"};

  static const std::unordered_set<std::string> kIntegerCommands = {"PushInt",
                                                                   "PushByte",
                                                                   "PushChar",
                                                                   "Rotate",
                                                                   "LoadLocal",
                                                                   "SetLocal",
                                                                   "LoadStatic",
                                                                   "SetStatic",
                                                                   "GetField",
                                                                   "SetField",
                                                                   "SleepMs",
                                                                   "SleepNs",
                                                                   "Exit",
                                                                   "SeedRandom"};

  static const std::unordered_set<std::string> kFloatCommands = {"PushFloat"};
  static const std::unordered_set<std::string> kBooleanCommands = {"PushBool"};
  static const std::unordered_set<std::string> kIdentCommands = {
      "Call", "CallVirtual", "CallConstructor", "SafeCall", "NewArray", "IsType", "SizeOf", "GetVTable", "SetVTable"};

  if (kStringCommands.contains(cmd)) {
    auto value = ctx.ConsumeStringLiteral();
    if (!value)
      throw value.error();
    auto exec = ovum::vm::execution_tree::CreateStringCommandByName(cmd, value.value());
    if (!exec)
      throw BytecodeParserError("Failed to create string command: " + cmd);
    block.AddStatement(std::move(exec.value()));
  } else if (kIntegerCommands.contains(cmd)) {
    auto value = ctx.ConsumeIntLiteral();
    if (!value)
      throw value.error();
    auto exec = ovum::vm::execution_tree::CreateIntegerCommandByName(cmd, value.value());
    if (!exec)
      throw BytecodeParserError("Failed to create integer command: " + cmd);
    block.AddStatement(std::move(exec.value()));
  } else if (kFloatCommands.contains(cmd)) {
    auto value = ctx.ConsumeFloatLiteral();
    if (!value)
      throw value.error();
    auto exec = ovum::vm::execution_tree::CreateFloatCommandByName(cmd, value.value());
    if (!exec)
      throw BytecodeParserError("Failed to create float command: " + cmd);
    block.AddStatement(std::move(exec.value()));
  } else if (kBooleanCommands.contains(cmd)) {
    auto value = ctx.ConsumeBoolLiteral();
    if (!value)
      throw value.error();
    auto exec = ovum::vm::execution_tree::CreateBooleanCommandByName(cmd, value.value());
    if (!exec)
      throw BytecodeParserError("Failed to create boolean command: " + cmd);
    block.AddStatement(std::move(exec.value()));
  } else if (kIdentCommands.contains(cmd)) {
    auto funcName = ctx.ConsumeIdentifier();
    if (!funcName)
      throw funcName.error();
    auto exec = ovum::vm::execution_tree::CreateStringCommandByName(cmd, funcName.value());
    if (!exec)
      throw BytecodeParserError("Failed to create call command: " + cmd);
    block.AddStatement(std::move(exec.value()));
  } else {
    auto exec = ovum::vm::execution_tree::CreateSimpleCommandByName(cmd);
    if (!exec)
      throw BytecodeParserError("Unknown command: " + cmd);
    block.AddStatement(std::move(exec.value()));
  }

  return true;
}

bool CommandParser::Handle(ParserContext& ctx) {
  return ParseSingleStatement(ctx, *ctx.current_block);
}

} // namespace ovum::bytecode::parser
