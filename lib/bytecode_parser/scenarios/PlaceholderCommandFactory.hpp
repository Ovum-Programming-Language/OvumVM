#ifndef BYTECODE_PARSER_PLACEHOLDERCOMMANDFACTORY_HPP_
#define BYTECODE_PARSER_PLACEHOLDERCOMMANDFACTORY_HPP_

#include <expected>
#include <memory>
#include <string>

#include "ICommandFactory.hpp"
#include "lib/execution_tree/IExecutable.hpp"

namespace ovum::bytecode::parser {

class PlaceholderCommandFactory : public ICommandFactory {
public:
  std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError> CreateCommand(
      const std::string& cmd_name, std::shared_ptr<ParsingSession> ctx) const override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_PLACEHOLDERCOMMANDFACTORY_HPP_
