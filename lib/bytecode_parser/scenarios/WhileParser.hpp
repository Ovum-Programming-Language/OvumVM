#ifndef BYTECODE_PARSER_WHILEPARSER_HPP_
#define BYTECODE_PARSER_WHILEPARSER_HPP_

#include "IParserHandler.hpp"

namespace ovum::bytecode::parser {

class WhileParser : public IParserHandler {
public:
  bool Handle(ParserContext& ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_WHILEPARSER_HPP_
