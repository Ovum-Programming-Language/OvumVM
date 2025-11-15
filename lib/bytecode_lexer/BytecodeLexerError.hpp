#ifndef BYTECODE_LEXER_BYTECODELEXERERROR_HPP_
#define BYTECODE_LEXER_BYTECODELEXERERROR_HPP_

#include <stdexcept>

namespace ovum::bytecode::lexer {

class BytecodeLexerError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

} // namespace ovum::bytecode::lexer

#endif // BYTECODE_LEXER_BYTECODELEXERERROR_HPP_
