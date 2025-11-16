#include <tokens/TokenFactory.hpp>

#include "NumberHandler.hpp"

#include <charconv>
#include <string>

namespace ovum::bytecode::lexer {

std::expected<std::optional<TokenPtr>, BytecodeLexerError> NumberHandler::Scan(BytecodeSourceWrapper& wrapper) {
  std::string num_str;
  num_str.push_back(wrapper.CurrentChar());
  bool has_dot = false;

  wrapper.ConsumeWhile(num_str, [&](char c) {
    if (c == '.') {
      if (has_dot)
        return false;
      has_dot = true;
      return true;
    }
    return c >= '0' && c <= '9';
  });

  if (num_str.empty() || (num_str == ".")) {
    return std::unexpected(BytecodeLexerError(std::string("Invalid number in line ") +
                                              std::to_string(wrapper.GetLine()) + std::string(" in column ") +
                                              std::to_string(wrapper.GetCol())));
  }

  if (!has_dot) {
    int64_t value;
    auto [ptr, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), value);
    if (ec != std::errc() || ptr != num_str.data() + num_str.size()) {
      return std::unexpected(BytecodeLexerError("Invalid integer literal: " + num_str));
    }
    return ovum::TokenFactory::MakeIntLiteral(num_str, value, wrapper.GetLine(), wrapper.GetTokenCol());
  } else {
    try {
      double value = std::stod(num_str);
      return ovum::TokenFactory::MakeFloatLiteral(num_str, value, wrapper.GetLine(), wrapper.GetTokenCol());
    } catch (const std::invalid_argument& e) {
      return std::unexpected(BytecodeLexerError("Invalid float literal: " + num_str));
    } catch (const std::out_of_range& e) {
      return std::unexpected(BytecodeLexerError("Float literal out of range: " + num_str));
    }
  }
}

} // namespace ovum::bytecode::lexer
