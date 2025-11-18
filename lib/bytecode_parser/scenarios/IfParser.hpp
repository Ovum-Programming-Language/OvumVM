#ifndef BYTECODE_PARSER_IFPARSER_HPP_
#define BYTECODE_PARSER_IFPARSER_HPP_

#include "IParserHandler.hpp"

namespace ovum::bytecode::parser {

class IfParser : public IParserHandler {
public:
  bool Handle(ParserContext& ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_IFPARSER_HPP_
