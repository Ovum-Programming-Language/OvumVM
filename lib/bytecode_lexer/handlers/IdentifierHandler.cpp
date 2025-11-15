#include <tokens/TokenFactory.hpp>

#include "IdentifierHandler.hpp"
#include "lib/bytecode_lexer/BytecodeSourceWrapper.hpp"

namespace ovum::bytecode::lexer {

OptToken IdentifierHandler::Scan(BytecodeSourceWrapper& wrapper) {
  std::string ident;
  ident.push_back(wrapper.CurrentChar());
  wrapper.ConsumeWhile(ident, [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '<' ||
           c == '>';
  });

  if (ident.empty()) {
    return std::unexpected(BytecodeLexerError("Empty identifier"));
  }

  if (BytecodeSourceWrapper::IsKeyword(ident)) {
    return ovum::TokenFactory::MakeKeyword(ident, wrapper.GetLine(), wrapper.GetTokenCol());
  }

  return ovum::TokenFactory::MakeIdent(ident, wrapper.GetLine(), wrapper.GetTokenCol());
}

} // namespace ovum::bytecode::lexer
