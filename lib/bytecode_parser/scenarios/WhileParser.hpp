#ifndef BYTECODE_PARSER_WHILEPARSER_HPP_
#define BYTECODE_PARSER_WHILEPARSER_HPP_

#include <expected>
#include <memory>

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "IParserHandler.hpp"

namespace ovum::bytecode::parser {

class WhileParser : public IParserHandler {
public:
  std::expected<void, BytecodeParserError> Handle(std::shared_ptr<ParserContext> ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_WHILEPARSER_HPP_
