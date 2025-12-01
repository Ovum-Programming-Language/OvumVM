#ifndef BYTECODE_PARSER_FUNCTIONPARSER_HPP_
#define BYTECODE_PARSER_FUNCTIONPARSER_HPP_

#include <expected>
#include <memory>

#include "IParserHandler.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParsingSession.hpp"

namespace ovum::bytecode::parser {

class FunctionParser : public IParserHandler {
public:
  std::expected<void, BytecodeParserError> Handle(std::shared_ptr<ParsingSession> ctx) override;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_FUNCTIONPARSER_HPP_
