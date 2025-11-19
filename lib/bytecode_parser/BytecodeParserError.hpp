#ifndef BYTECODE_PARSER_BYTECODEPARSERERROR_HPP_
#define BYTECODE_PARSER_BYTECODEPARSERERROR_HPP_

#include <stdexcept>
#include <string>

namespace ovum::bytecode::parser {

class BytecodeParserError : public std::runtime_error {
public:
  explicit BytecodeParserError(const std::string& message) : std::runtime_error(message.c_str()) {
  }
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_BYTECODEPARSERERROR_HPP_
