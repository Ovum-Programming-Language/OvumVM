#include <tokens/TokenFactory.hpp>

#include "StringHandler.hpp"
#include "lib/bytecode_lexer/BytecodeLexerError.hpp"

namespace ovum::bytecode::lexer {

OptToken StringHandler::Scan(BytecodeSourceWrapper& wrapper) {
  wrapper.Advance();

  std::string content;
  bool escaped = false;

  while (!wrapper.IsAtEnd()) {
    char c = wrapper.Peek();
    if (c == '\0')
      break;

    if (escaped) {
      if (c == '"' || c == '\\') {
        content += c;
      } else {
        content += '\\';
        content += c;
      }
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      wrapper.Advance();
      break;
    } else {
      content += c;
    }
    wrapper.Advance();
  }

  if (wrapper.Peek() != '"') {
    return std::unexpected(BytecodeLexerError("Unterminated string literal"));
  }

  std::string raw_lexeme = "\"" + content + "\"";
  return ovum::TokenFactory::MakeStringLiteral(raw_lexeme, content, wrapper.GetLine(), wrapper.GetTokenCol());
}

} // namespace ovum::bytecode::lexer
