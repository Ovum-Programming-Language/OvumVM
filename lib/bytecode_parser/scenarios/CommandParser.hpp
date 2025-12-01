#ifndef BYTECODE_PARSER_COMMANDPARSER_HPP_
#define BYTECODE_PARSER_COMMANDPARSER_HPP_

#include <expected>
#include <memory>

#include "ICommandFactory.hpp"
#include "IParserHandler.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParsingSession.hpp"
#include "lib/execution_tree/Block.hpp"

namespace ovum::bytecode::parser {

class CommandParser : public IParserHandler {
public:
  explicit CommandParser(std::unique_ptr<ICommandFactory> factory = nullptr);

  std::expected<void, BytecodeParserError> Handle(std::shared_ptr<ParsingSession> ctx) override;

  static std::expected<void, BytecodeParserError> ParseSingleStatement(const std::shared_ptr<ParsingSession>& ctx,
                                                                       vm::execution_tree::Block& block,
                                                                       ICommandFactory& factory);

  static std::expected<void, BytecodeParserError> ParseSingleStatement(std::shared_ptr<ParsingSession> ctx,
                                                                       vm::execution_tree::Block& block);

  static ICommandFactory& DefaultFactory();

private:
  std::unique_ptr<ICommandFactory> factory_;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_COMMANDPARSER_HPP_
