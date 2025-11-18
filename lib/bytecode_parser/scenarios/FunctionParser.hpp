#ifndef BYTECODE_PARSER_FUNCTIONPARSER_HPP_
#define BYTECODE_PARSER_FUNCTIONPARSER_HPP_

#include "IParserHandler.hpp"

namespace ovum::bytecode::parser {

class FunctionParser : public IParserHandler {
public:
  bool Handle(ParserContext& ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_FUNCTIONPARSER_HPP_
