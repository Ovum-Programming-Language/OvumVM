#include "CommandFactory.hpp"

#include "lib/execution_tree/command_factory.hpp"

#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

const std::unordered_set<std::string> CommandFactory::kStringCommands = {"PushString", "PushChar"};

const std::unordered_set<std::string> CommandFactory::kIntegerCommands = {
    "PushInt", "PushByte", "Rotate", "LoadLocal", "SetLocal", "LoadStatic", "SetStatic", "GetField", "SetField"};

const std::unordered_set<std::string> CommandFactory::kFloatCommands = {"PushFloat"};

const std::unordered_set<std::string> CommandFactory::kBooleanCommands = {"PushBool"};

const std::unordered_set<std::string> CommandFactory::kIdentCommands = {
    "NewArray", "Call", "CallVirtual", "CallConstructor", "GetVTable", "SetVTable", "SafeCall", "IsType", "SizeOf"};

std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError> CommandFactory::CreateCommand(
    const std::string& cmd_name, std::shared_ptr<ParsingSession> ctx) {
  if (kStringCommands.contains(cmd_name)) {
    std::expected<std::string, BytecodeParserError> value = ctx->ConsumeStringLiteral();

    if (!value) {
      return std::unexpected(value.error());
    }

    std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> cmd =
        vm::execution_tree::CreateStringCommandByName(cmd_name, value.value());

    if (!cmd) {
      return std::unexpected(BytecodeParserError("Failed to create string command: " + cmd_name));
    }

    return std::move(cmd.value());
  }

  if (kIntegerCommands.contains(cmd_name)) {
    std::expected<int64_t, BytecodeParserError> value = ctx->ConsumeIntLiteral();

    if (!value) {
      return std::unexpected(value.error());
    }

    std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> cmd =
        vm::execution_tree::CreateIntegerCommandByName(cmd_name, value.value());

    if (!cmd) {
      return std::unexpected(BytecodeParserError("Failed to create integer command: " + cmd_name));
    }

    return std::move(cmd.value());
  }

  if (kFloatCommands.contains(cmd_name)) {
    std::expected<double, BytecodeParserError> value = ctx->ConsumeFloatLiteral();

    if (!value) {
      return std::unexpected(value.error());
    }

    std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> cmd =
        vm::execution_tree::CreateFloatCommandByName(cmd_name, value.value());

    if (!cmd) {
      return std::unexpected(BytecodeParserError("Failed to create float command: " + cmd_name));
    }

    return std::move(cmd.value());
  }

  if (kBooleanCommands.contains(cmd_name)) {
    std::expected<bool, BytecodeParserError> value = ctx->ConsumeBoolLiteral();

    if (!value) {
      return std::unexpected(value.error());
    }

    std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> cmd =
        vm::execution_tree::CreateBooleanCommandByName(cmd_name, value.value());

    if (!cmd) {
      return std::unexpected(BytecodeParserError("Failed to create boolean command: " + cmd_name));
    }

    return std::move(cmd.value());
  }

  if (kIdentCommands.contains(cmd_name)) {
    std::expected<std::string, BytecodeParserError> ident = ctx->ConsumeIdentifier();

    if (!ident) {
      return std::unexpected(ident.error());
    }

    std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> cmd =
        vm::execution_tree::CreateStringCommandByName(cmd_name, ident.value());

    if (!cmd) {
      return std::unexpected(BytecodeParserError("Failed to create identifier command: " + cmd_name));
    }

    return std::move(cmd.value());
  }

  std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> cmd =
      vm::execution_tree::CreateSimpleCommandByName(cmd_name);

  if (!cmd) {
    return std::unexpected(BytecodeParserError("Unknown or unimplemented command: " + cmd_name));
  }

  return std::move(cmd.value());
}

} // namespace ovum::bytecode::parser
