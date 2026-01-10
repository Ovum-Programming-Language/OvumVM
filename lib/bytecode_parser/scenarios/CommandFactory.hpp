#ifndef BYTECODE_PARSER_COMMANDFACTORY_HPP_
#define BYTECODE_PARSER_COMMANDFACTORY_HPP_

#include <expected>
#include <memory>
#include <string>
#include <unordered_set>

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParsingSession.hpp"
#include "lib/execution_tree/IExecutable.hpp"

#include "ICommandFactory.hpp"

namespace ovum::bytecode::parser {

class CommandFactory : public ICommandFactory {
public:
  CommandFactory() = default;

  std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError> CreateCommand(
      const std::string& cmd_name, std::shared_ptr<ParsingSession> ctx) const override;

private:
  static const std::unordered_set<std::string> kStringCommands;
  static const std::unordered_set<std::string> kIntegerCommands;
  static const std::unordered_set<std::string> kFloatCommands;
  static const std::unordered_set<std::string> kBooleanCommands;
  static const std::unordered_set<std::string> kIdentCommands;
  static const std::unordered_set<std::string> kCharCommands;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_COMMANDFACTORY_HPP_
