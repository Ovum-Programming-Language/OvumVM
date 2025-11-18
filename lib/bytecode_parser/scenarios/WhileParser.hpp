#ifndef BYTECODE_PARSER_WHILEPARSER_HPP_
#define BYTECODE_PARSER_WHILEPARSER_HPP_

#include "IParserHandler.hpp"

#include <expected>

namespace ovum::bytecode::parser {

class WhileParser : public IParserHandler {
public:
  std::expected<void, BytecodeParserError> Handle(ParserContext& ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_WHILEPARSER_HPP_
