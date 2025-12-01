#ifndef BYTECODE_PARSER_BYTECODEPARSERERROR_HPP_
#define BYTECODE_PARSER_BYTECODEPARSERERROR_HPP_

#include <stdexcept>
#include <string>
#include "BytecodeParserErrorCode.hpp"

namespace ovum::bytecode::parser {

class BytecodeParserError : public std::runtime_error {
public:
  explicit BytecodeParserError(const std::string& message,
                               BytecodeParserErrorCode code = BytecodeParserErrorCode::kGeneric) :
      std::runtime_error(message), code_(code) {
  }

  [[nodiscard]] BytecodeParserErrorCode Code() const {
    return code_;
  }

private:
  BytecodeParserErrorCode code_;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_BYTECODEPARSERERROR_HPP_
