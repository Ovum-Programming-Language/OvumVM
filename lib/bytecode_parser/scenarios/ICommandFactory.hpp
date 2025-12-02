#ifndef BYTECODE_PARSER_ICOMMANDFACTORY_HPP_
#define BYTECODE_PARSER_ICOMMANDFACTORY_HPP_

#include <expected>
#include <memory>

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParsingSession.hpp"
#include "lib/execution_tree/IExecutable.hpp"

namespace ovum::bytecode::parser {

class ICommandFactory { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~ICommandFactory() = default;

  virtual std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, BytecodeParserError> CreateCommand(
      const std::string& cmd_name, std::shared_ptr<ParsingSession> ctx) = 0;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_ICOMMANDFACTORY_HPP_
