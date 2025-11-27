#ifndef BYTECODE_PARSER_FUNCTIONPARSER_HPP_
#define BYTECODE_PARSER_FUNCTIONPARSER_HPP_

#include <expected>

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"

#include "IParserHandler.hpp"

namespace ovum::bytecode::parser {

class FunctionParser : public IParserHandler {
public:
  std::expected<void, BytecodeParserError> Handle(ParserContext& ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_FUNCTIONPARSER_HPP_
