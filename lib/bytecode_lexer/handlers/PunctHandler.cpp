#include <tokens/TokenFactory.hpp>

#include "PunctHandler.hpp"

namespace ovum::bytecode::lexer {

OptToken PunctHandler::Scan(BytecodeSourceWrapper& wrapper) {
  char punct = wrapper.CurrentChar();

  if (punct != '{' && punct != '}' && punct != ',' && punct != ';' && punct != '(' && punct != ')' && punct != '[' &&
      punct != ']' && punct != ':' && punct != '@') {
    return std::unexpected(BytecodeLexerError("Invalid punctuation character: " + std::string(1, punct)));
  }

  std::string lex(1, punct);
  auto token = ovum::TokenFactory::MakePunct(lex, wrapper.GetLine(), wrapper.GetTokenCol());
  return token;
}

} // namespace ovum::bytecode::lexer
