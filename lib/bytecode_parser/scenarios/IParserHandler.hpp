#ifndef BYTECODE_PARSER_IPARSERHANDLER_HPP_
#define BYTECODE_PARSER_IPARSERHANDLER_HPP_

#include <expected>
#include <memory>

#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

class ParsingSession;

class IParserHandler { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IParserHandler() = default;
  virtual std::expected<void, BytecodeParserError> Handle(std::shared_ptr<ParsingSession> ctx) = 0;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_IPARSERHANDLER_HPP_
