#ifndef BYTECODE_PARSER_COMMANDPARSER_HPP_
#define BYTECODE_PARSER_COMMANDPARSER_HPP_

#include "IParserHandler.hpp"
#include "lib/execution_tree/Block.hpp"

namespace ovum::bytecode::parser {

class CommandParser : public IParserHandler {
public:
  bool Handle(ParserContext& ctx) override;
  static bool ParseSingleStatement(ParserContext& ctx, vm::execution_tree::Block& block);
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_COMMANDPARSER_HPP_
