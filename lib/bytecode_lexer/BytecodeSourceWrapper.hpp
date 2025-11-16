#ifndef BYTECODE_LEXER_BYTECODESOURCEWRAPPER_HPP_
#define BYTECODE_LEXER_BYTECODESOURCEWRAPPER_HPP_

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>

namespace ovum::bytecode::lexer {

class BytecodeSourceWrapper {
public:
  BytecodeSourceWrapper(std::string_view src);

  [[nodiscard]] bool IsAtEnd() const noexcept;

  [[nodiscard]] char Peek(size_t offset = 0) const noexcept;

  [[nodiscard]] char CurrentChar() const noexcept;

  char Advance();

  void RetreatOne();

  void ConsumeWhile(std::string& out, const std::function<bool(char)>& pred);

  void ResetTokenPosition();

  [[nodiscard]] int32_t GetLine() const noexcept;

  [[nodiscard]] int32_t GetCol() const noexcept;

  [[nodiscard]] int32_t GetTokenCol() const noexcept;

  [[nodiscard]] static bool IsKeyword(std::string_view s);

private:
  std::string_view src_;

  size_t start_{0};
  size_t current_{0};
  int32_t line_{1};
  int32_t col_{1};
  int32_t token_col_{1};

  static const std::unordered_set<std::string_view> kBytecodeKeywords;
};

} // namespace ovum::bytecode::lexer

#endif // BYTECODE_LEXER_BYTECODESOURCEWRAPPER_HPP_
