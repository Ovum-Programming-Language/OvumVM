#ifndef EXECUTION_TREE_COMMANDFACTORY_HPP
#define EXECUTION_TREE_COMMANDFACTORY_HPP

#include <cstdint>
#include <expected>
#include <memory>
#include <stdexcept>
#include <string>

#include "IExecutable.hpp"

namespace ovum::vm::execution_tree {

/**
 * Creates a simple (no bytecode arguments) command by name.
 * @param name The name of the command.
 * @return The command unique pointer or std::out_of_range if the command is not found.
 */
std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateSimpleCommandByName(const std::string& name);

/**
 * Creates a string-argument command by name (Ovum type String or an identifier).
 * @param name The name of the command.
 * @param value The value of the command.
 * @return The command unique pointer or std::out_of_range if the command is not found.
 */
std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateStringCommandByName(const std::string& name, const std::string& value);

/**
 * Creates an integer-argument command by name (Ovum types Int, Char, Byte or an index).
 * @param name The name of the command.
 * @param value The value of the command.
 * @return The command unique pointer or std::out_of_range if the command is not found.
 */
std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateIntegerCommandByName(const std::string& name, const int64_t value);

/**
 * Creates a floating point-argument command by name (Ovum type Double).
 * @param name The name of the command.
 * @param value The value of the command.
 * @return The command unique pointer or std::out_of_range if the command is not found.
 */
std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateFloatCommandByName(const std::string& name, const double value);

/**
 * Creates a boolean-argument command by name (Ovum type Bool).
 * @param name The name of the command.
 * @param value The value of the command.
 * @return The command unique pointer or std::out_of_range if the command is not found.
 */
std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateBooleanCommandByName(const std::string& name, const bool value);

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_COMMANDFACTORY_HPP
