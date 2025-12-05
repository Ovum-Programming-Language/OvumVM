#include "PlaceholderCommandFactory.hpp"

#include "lib/execution_tree/Command.hpp"

namespace ovum::bytecode::parser {

static const auto kStubCommand = [](const std::string& full_name) -> std::unique_ptr<vm::execution_tree::IExecutable> {
  auto cmd_func = [full_name](vm::execution_tree::PassedExecutionData&)
      -> std::expected<vm::execution_tree::ExecutionResult, std::runtime_error> {
    return std::unexpected(std::runtime_error("Command not implemented: " + full_name));
  };
  return std::make_unique<vm::execution_tree::Command<decltype(cmd_func)>>(std::move(cmd_func));
};

std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError>
PlaceholderCommandFactory::CreateCommand(const std::string& cmd_name, std::shared_ptr<ParsingSession>) const {
  return kStubCommand(cmd_name);
}

} // namespace ovum::bytecode::parser
