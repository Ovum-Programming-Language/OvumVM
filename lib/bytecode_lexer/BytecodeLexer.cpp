#include "BytecodeLexer.hpp"

#include <utility>

#include <tokens/TokenFactory.hpp>

#include "handlers/DefaultHandler.hpp"
#include "handlers/IdentifierHandler.hpp"
#include "handlers/NewlineHandler.hpp"
#include "handlers/NumberHandler.hpp"
#include "handlers/PunctHandler.hpp"
#include "handlers/StringHandler.hpp"
#include "handlers/WhitespaceHandler.hpp"

namespace ovum::bytecode::lexer {

namespace {
constexpr const char* kBytecodePunctChars = "{},;()[]:@";
} // namespace

BytecodeLexer::BytecodeLexer(std::string_view src) :
    wrapper_(src), handlers_(MakeDefaultBytecodeHandlers()), default_handler_(MakeDefaultBytecodeHandler()) {
}

std::expected<std::vector<TokenPtr>, BytecodeLexerError> BytecodeLexer::Tokenize() {
  std::vector<TokenPtr> tokens;
  tokens.reserve(kDefaultBytecodeTokenReserve);

  while (!wrapper_.IsAtEnd()) {
    wrapper_.ResetTokenPosition();

    char ch_read = wrapper_.Advance();
    Handler* current_handler = handlers_.at(static_cast<unsigned char>(ch_read)).get();

    if (!current_handler) {
      current_handler = default_handler_.get();
    }

    auto maybe_token_result = current_handler->Scan(wrapper_);

    if (!maybe_token_result) {
      return std::unexpected(BytecodeLexerError(maybe_token_result.error().what()));
    }

    auto maybe_token = maybe_token_result.value();

    if (maybe_token && *maybe_token) {
      tokens.push_back(std::move(*maybe_token));
    }
  }

  tokens.push_back(ovum::TokenFactory::MakeEof(wrapper_.GetLine(), wrapper_.GetCol()));
  return tokens;
}

void BytecodeLexer::SetHandler(unsigned char ch, std::unique_ptr<Handler> handler) {
  handlers_.at(ch) = std::move(handler);
}

void BytecodeLexer::SetDefaultHandler(std::unique_ptr<Handler> handler) {
  default_handler_ = std::move(handler);
}

std::array<std::unique_ptr<Handler>, kDefaultBytecodeTokenReserve> BytecodeLexer::MakeDefaultBytecodeHandlers() {
  std::array<std::unique_ptr<Handler>, kDefaultBytecodeTokenReserve> table{};

  table.at(static_cast<unsigned char>(' ')) = std::make_unique<WhitespaceHandler>();
  table.at(static_cast<unsigned char>('\t')) = std::make_unique<WhitespaceHandler>();
  table.at(static_cast<unsigned char>('\r')) = std::make_unique<WhitespaceHandler>();

  table.at(static_cast<unsigned char>('\n')) = std::make_unique<NewlineHandler>();

  for (unsigned char ch = 'a'; ch <= 'z'; ++ch) {
    table.at(ch) = std::make_unique<IdentifierHandler>();
  }
  for (unsigned char ch = 'A'; ch <= 'Z'; ++ch) {
    table.at(ch) = std::make_unique<IdentifierHandler>();
  }
  table.at(static_cast<unsigned char>('_')) = std::make_unique<IdentifierHandler>();

  for (unsigned char digit = '0'; digit <= '9'; ++digit) {
    table.at(digit) = std::make_unique<NumberHandler>();
  }

  table.at(static_cast<unsigned char>('"')) = std::make_unique<StringHandler>();

  for (const unsigned char ch : std::string(kBytecodePunctChars)) {
    table.at(ch) = std::make_unique<PunctHandler>();
  }

  return table;
}

std::unique_ptr<Handler> BytecodeLexer::MakeDefaultBytecodeHandler() {
  return std::make_unique<DefaultHandler>();
}

} // namespace ovum::bytecode::lexer
