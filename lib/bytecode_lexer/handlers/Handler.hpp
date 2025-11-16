#ifndef BYTECODE_LEXER_HANDLERS_HANDLER_HPP_
#define BYTECODE_LEXER_HANDLERS_HANDLER_HPP_

#include <expected>
#include <memory>
#include <string>

#include "lib/bytecode_lexer/BytecodeLexerError.hpp"
#include "lib/bytecode_lexer/BytecodeSourceWrapper.hpp"
#include "tokens/Token.hpp"

namespace ovum::bytecode::lexer {

using TokenPtr = ovum::TokenPtr;

class Handler {
public:
  virtual ~Handler() = default;

  [[nodiscard]] virtual std::expected<std::optional<TokenPtr>, BytecodeLexerError> Scan(BytecodeSourceWrapper& wrapper) = 0;
};

} // namespace ovum::bytecode::lexer

#endif // BYTECODE_LEXER_HANDLERS_HANDLER_HPP_
