#include "PlaceholderCommandFactory.hpp"

#include <unordered_set>

#include "lib/execution_tree/Command.hpp"

namespace ovum::bytecode::parser {

namespace {
const std::unordered_set<std::string> kStringCommands = {"PushString", "PushChar"};

const std::unordered_set<std::string> kIntegerCommands = {
    "PushInt", "PushByte", "Rotate", "LoadLocal", "SetLocal", "LoadStatic", "SetStatic", "GetField", "SetField"};

const std::unordered_set<std::string> kFloatCommands = {"PushFloat"};

const std::unordered_set<std::string> kBooleanCommands = {"PushBool"};

const std::unordered_set<std::string> kIdentCommands = {
    "NewArray", "Call", "CallVirtual", "CallConstructor", "GetVTable", "SetVTable", "SafeCall", "IsType", "SizeOf"};

static const auto kStubCommand = [](const std::string& full_name) -> std::unique_ptr<vm::execution_tree::IExecutable> {
  auto cmd_func = [full_name](vm::execution_tree::PassedExecutionData&)
      -> std::expected<vm::execution_tree::ExecutionResult, std::runtime_error> {
    return std::unexpected(std::runtime_error("Command not implemented: " + full_name));
  };
  return std::make_unique<vm::execution_tree::Command<decltype(cmd_func)>>(std::move(cmd_func));
};
} // namespace

std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError>
PlaceholderCommandFactory::CreateCommand(const std::string& cmd_name, std::shared_ptr<ParsingSession> ctx) const {
  // Consume arguments based on command type (same logic as CommandFactory)
  // This ensures the parser advances correctly through tokens
  if (kStringCommands.contains(cmd_name)) {
    std::expected<std::string, BytecodeParserError> value = ctx->ConsumeStringLiteral();
    if (!value) {
      return std::unexpected(value.error());
    }
    return kStubCommand(cmd_name);
  }

  if (kIntegerCommands.contains(cmd_name)) {
    std::expected<int64_t, BytecodeParserError> value = ctx->ConsumeIntLiteral();
    if (!value) {
      return std::unexpected(value.error());
    }
    return kStubCommand(cmd_name);
  }

  if (kFloatCommands.contains(cmd_name)) {
    std::expected<double, BytecodeParserError> value = ctx->ConsumeFloatLiteral();
    if (!value) {
      return std::unexpected(value.error());
    }
    return kStubCommand(cmd_name);
  }

  if (kBooleanCommands.contains(cmd_name)) {
    std::expected<bool, BytecodeParserError> value = ctx->ConsumeBoolLiteral();
    if (!value) {
      return std::unexpected(value.error());
    }
    return kStubCommand(cmd_name);
  }

  if (kIdentCommands.contains(cmd_name)) {
    std::expected<std::string, BytecodeParserError> ident = ctx->ConsumeIdentifier();
    if (!ident) {
      return std::unexpected(ident.error());
    }
    return kStubCommand(cmd_name);
  }

  // Simple commands (no arguments)
  return kStubCommand(cmd_name);
}

} // namespace ovum::bytecode::parser
