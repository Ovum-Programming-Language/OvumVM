#include "command_factory.hpp"

namespace ovum::vm::execution_tree {

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateSimpleCommandByName(const std::string& name) {
  return std::unexpected(std::out_of_range("Command not found: " + name));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateStringCommandByName(const std::string& name, const std::string& value) {
  return std::unexpected(std::out_of_range("Command not found: " + name));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateIntegerCommandByName(const std::string& name, const int64_t value) {
  return std::unexpected(std::out_of_range("Command not found: " + name));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateFloatCommandByName(const std::string& name, const double value) {
  return std::unexpected(std::out_of_range("Command not found: " + name));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateBooleanCommandByName(const std::string& name, const bool value) {
  return std::unexpected(std::out_of_range("Command not found: " + name));
}

} // namespace ovum::vm::execution_tree