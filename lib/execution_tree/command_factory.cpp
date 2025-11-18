#include <iostream>
#include "command_factory.hpp"

#include "lib/execution_tree/Command.hpp"

namespace ovum::vm::execution_tree {

auto StubCommand = [](std::string full_name) -> std::unique_ptr<IExecutable> {
  auto cmd_func = [full_name](PassedExecutionData&) -> std::expected<ExecutionResult, std::runtime_error> {
    std::cerr << "STUB: unimplemented command '" << full_name << "'\n";
    return std::unexpected(std::runtime_error("Command not implemented: " + full_name));
  };

  return std::make_unique<Command<decltype(cmd_func)>>(std::move(cmd_func));
};

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateSimpleCommandByName(const std::string& name) {
  std::cerr << "STUB command: " << name << "\n";
  return StubCommand(name);
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateStringCommandByName(const std::string& name,
                                                                                         const std::string& value) {
  std::cerr << "STUB command: " << name << " \"" << value << "\"\n";
  return StubCommand(name + " \"" + value + "\"");
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateIntegerCommandByName(const std::string& name,
                                                                                          const int64_t value) {
  std::cerr << "STUB command: " << name << " " << value << "\n";
  return StubCommand(name + " " + std::to_string(value));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateFloatCommandByName(const std::string& name,
                                                                                        const double value) {
  std::cerr << "STUB command: " << name << " " << value << "\n";
  return StubCommand(name + " " + std::to_string(value));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateBooleanCommandByName(const std::string& name,
                                                                                          const bool value) {
  std::cerr << "STUB command: " << name << " " << (value ? "true" : "false") << "\n";
  return StubCommand(name + " " + (value ? "true" : "false"));
}

} // namespace ovum::vm::execution_tree
