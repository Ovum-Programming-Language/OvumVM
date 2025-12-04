#ifndef BYTECODE_PARSER_VTABLEPARSER_HPP_
#define BYTECODE_PARSER_VTABLEPARSER_HPP_

#include <expected>
#include <memory>

#include "IParserHandler.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParsingSession.hpp"

namespace ovum::bytecode::parser {

class VtableParser : public IParserHandler {
public:
  std::expected<bool, BytecodeParserError> Handle(std::shared_ptr<ParsingSession> ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_VTABLEPARSER_HPP_
