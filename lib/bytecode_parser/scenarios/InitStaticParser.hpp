#ifndef BYTECODE_PARSER_INITSTATICPARSER_HPP_
#define BYTECODE_PARSER_INITSTATICPARSER_HPP_

#include "IParserHandler.hpp"

#include <expected>

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"

namespace ovum::bytecode::parser {

class InitStaticParser : public IParserHandler {
public:
  std::expected<void, BytecodeParserError> Handle(ParserContext& ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_INITSTATICPARSER_HPP_
