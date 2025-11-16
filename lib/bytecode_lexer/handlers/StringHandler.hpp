#ifndef BYTECODE_LEXER_HANDLERS_BYTECODESTRINGHANDLER_HPP_
#define BYTECODE_LEXER_HANDLERS_BYTECODESTRINGHANDLER_HPP_

#include "Handler.hpp"

namespace ovum::bytecode::lexer {

class StringHandler final : public Handler {
public:
  [[nodiscard]] std::expected<std::optional<TokenPtr>, BytecodeLexerError> Scan(BytecodeSourceWrapper& wrapper) override;
};

} // namespace ovum::bytecode::lexer

#endif // BYTECODE_LEXER_HANDLERS_BYTECODESTRINGHANDLER_HPP_
