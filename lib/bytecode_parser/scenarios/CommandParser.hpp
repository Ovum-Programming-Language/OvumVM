#ifndef BYTECODE_PARSER_COMMANDPARSER_HPP_
#define BYTECODE_PARSER_COMMANDPARSER_HPP_

#include <expected>

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/Block.hpp"

#include "IParserHandler.hpp"

namespace ovum::bytecode::parser {

class CommandParser : public IParserHandler {
public:
  std::expected<void, BytecodeParserError> Handle(ParserContext& ctx) override;
  static std::expected<void, BytecodeParserError> ParseSingleStatement(ParserContext& ctx,
                                                                       vm::execution_tree::Block& block);
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_COMMANDPARSER_HPP_
