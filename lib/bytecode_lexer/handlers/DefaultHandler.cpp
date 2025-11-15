#include "DefaultHandler.hpp"
#include "lib/bytecode_lexer/BytecodeLexerError.hpp"

namespace ovum::bytecode::lexer {

OptToken DefaultHandler::Scan(BytecodeSourceWrapper& wrapper) {
  char invalid = wrapper.CurrentChar();
  return std::unexpected(BytecodeLexerError(std::string("Unexpected character: '") + invalid + "'"));
}

} // namespace ovum::bytecode::lexer
