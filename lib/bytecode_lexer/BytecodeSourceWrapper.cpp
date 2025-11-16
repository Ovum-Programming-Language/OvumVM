#include "BytecodeSourceWrapper.hpp"

#include <string_view>

namespace ovum::bytecode::lexer {

const std::unordered_set<std::string_view> BytecodeSourceWrapper::kBytecodeKeywords = {"if",
                                                                                       "then",
                                                                                       "else",
                                                                                       "while",
                                                                                       "init-static",
                                                                                       "function",
                                                                                       "pure",
                                                                                       "no-jit",
                                                                                       "vtable",
                                                                                       "size",
                                                                                       "interfaces",
                                                                                       "methods",
                                                                                       "vartable",
                                                                                       "true",
                                                                                       "false"};

BytecodeSourceWrapper::BytecodeSourceWrapper(std::string_view src) : src_(src) {
}

bool BytecodeSourceWrapper::IsAtEnd() const noexcept {
  return current_ >= src_.size();
}

char BytecodeSourceWrapper::Peek(size_t offset) const noexcept {
  size_t idx = current_ + offset;

  if (idx >= src_.size()) {
    return '\0';
  }

  return src_[idx];
}

char BytecodeSourceWrapper::CurrentChar() const noexcept {
  if (current_ == 0) {
    return '\0';
  }

  return src_[current_ - 1];
}

char BytecodeSourceWrapper::Advance() {
  if (IsAtEnd()) {
    return '\0';
  }

  char c = src_[current_++];

  if (c == '\n') {
    ++line_;
    col_ = 1;
  } else {
    ++col_;
  }

  return c;
}

void BytecodeSourceWrapper::ConsumeWhile(std::string& out, const std::function<bool(char)>& pred) {
  while (!IsAtEnd() && pred(Peek())) {
    out.push_back(Advance());
  }
}

void BytecodeSourceWrapper::ResetTokenPosition() {
  start_ = current_;
  token_col_ = col_;
}

int32_t BytecodeSourceWrapper::GetLine() const noexcept {
  return line_;
}

int32_t BytecodeSourceWrapper::GetCol() const noexcept {
  return col_;
}

int32_t BytecodeSourceWrapper::GetTokenCol() const noexcept {
  return token_col_;
}

bool BytecodeSourceWrapper::IsKeyword(std::string_view s) {
  return kBytecodeKeywords.contains(s);
}

} // namespace ovum::bytecode::lexer
