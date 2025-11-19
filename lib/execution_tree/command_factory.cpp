#include <iostream>
#include "command_factory.hpp"

#include "lib/execution_tree/Command.hpp"

namespace ovum::vm::execution_tree {

const auto stub_command = [](std::string full_name) -> std::unique_ptr<IExecutable> {
  auto cmd_func = [full_name](PassedExecutionData&) -> std::expected<ExecutionResult, std::runtime_error> {
    std::cerr << "STUB: unimplemented command '" << full_name << "'\n";
    return std::unexpected(std::runtime_error("Command not implemented: " + full_name));
  };

  return std::make_unique<Command<decltype(cmd_func)>>(std::move(cmd_func));
};

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateSimpleCommandByName(const std::string& name) {
  return stub_command(name);
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateStringCommandByName(const std::string& name,
                                                                                         const std::string& value) {
  return stub_command(name + " \"" + value + "\"");
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateIntegerCommandByName(const std::string& name,
                                                                                          const int64_t value) {
  return stub_command(name + " " + std::to_string(value));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateFloatCommandByName(const std::string& name,
                                                                                        const double value) {
  return stub_command(name + " " + std::to_string(value));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateBooleanCommandByName(const std::string& name,
                                                                                          const bool value) {
  return stub_command(name + " " + (value ? "true" : "false"));
}

} // namespace ovum::vm::execution_tree
