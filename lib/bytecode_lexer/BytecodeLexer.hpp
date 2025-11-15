#ifndef BYTECODE_LEXER_BYTECODELEXER_HPP_
#define BYTECODE_LEXER_BYTECODELEXER_HPP_

#include <array>
#include <cstddef>
#include <expected>
#include <memory>
#include <string_view>
#include <vector>

#include <tokens/Token.hpp>

#include "BytecodeLexerError.hpp"
#include "BytecodeSourceWrapper.hpp"
#include "handlers/Handler.hpp"

namespace ovum::bytecode::lexer {

constexpr std::size_t kDefaultBytecodeTokenReserve = 512;

class BytecodeLexer {
public:
  explicit BytecodeLexer(std::string_view src);

  std::expected<std::vector<TokenPtr>, BytecodeLexerError> Tokenize();

  void SetHandler(unsigned char ch, std::unique_ptr<Handler> handler);

  void SetDefaultHandler(std::unique_ptr<Handler> handler);

private:
  static std::array<std::unique_ptr<Handler>, kDefaultBytecodeTokenReserve> MakeDefaultBytecodeHandlers();

  static std::unique_ptr<Handler> MakeDefaultBytecodeHandler();

  BytecodeSourceWrapper wrapper_;
  std::array<std::unique_ptr<Handler>, kDefaultBytecodeTokenReserve> handlers_{};
  std::unique_ptr<Handler> default_handler_;
};

} // namespace ovum::bytecode::lexer

#endif // BYTECODE_LEXER_BYTECODELEXER_HPP_
