#include "WhitespaceHandler.hpp"

namespace ovum::bytecode::lexer {

std::expected<std::optional<TokenPtr>, BytecodeLexerError> WhitespaceHandler::Scan(BytecodeSourceWrapper& wrapper) {
  std::string ws;
  ws.push_back(wrapper.CurrentChar());
  wrapper.ConsumeWhile(ws, [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; });

  return std::nullopt;
}

} // namespace ovum::bytecode::lexer
