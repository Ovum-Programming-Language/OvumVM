#include "ParserContext.hpp"

#include <tokens/EofToken.hpp>
#include <tokens/IdentToken.hpp>
#include <tokens/KeywordToken.hpp>
#include <tokens/LiteralToken.hpp>
#include <tokens/PunctToken.hpp>

namespace ovum::bytecode::parser {

ParserContext::ParserContext(const std::vector<TokenPtr>& tokens,
                             vm::execution_tree::FunctionRepository& func_repo,
                             vm::runtime::VirtualTableRepository& vtable_repo,
                             vm::runtime::RuntimeMemory& memory,
                             std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory,
                             size_t jit_boundary) :
    tokens_(tokens), func_repo(func_repo), vtable_repo(vtable_repo), memory(memory),
    jit_factory(jit_factory), jit_boundary(jit_boundary), init_static_block(nullptr) {
}

const TokenPtr ParserContext::Current() const {
  if (pos_ >= tokens_.size()) {
    static const EofToken kEof(TokenPosition(0, 0));

    return std::make_shared<EofToken>(kEof);
  }

  return tokens_[pos_];
}

bool ParserContext::IsEof() const {
  return pos_ >= tokens_.size() || tokens_[pos_]->GetStringType() == "EOF";
}

void ParserContext::Advance() {
  if (!IsEof()) {
    ++pos_;
  }
}

bool ParserContext::IsIdentifier() const {
  return Current()->GetStringType() == "IDENT";
}

bool ParserContext::IsKeyword(const std::string& kw) const {
  return Current()->GetStringType() == "KEYWORD" && Current()->GetLexeme() == kw;
}

bool ParserContext::IsPunct(char ch) const {
  return Current()->GetStringType() == "PUNCT" && Current()->GetLexeme() == std::string(1, ch);
}

bool ParserContext::IsPunct(const std::string& p) const {
  return Current()->GetStringType() == "PUNCT" && Current()->GetLexeme() == p;
}

std::expected<void, BytecodeParserError> ParserContext::ExpectKeyword(const std::string& kw) {
  if (!IsKeyword(kw)) {
    auto pos = Current()->GetPosition();

    return std::unexpected(BytecodeParserError("Expected keyword '" + kw + "' at " + std::to_string(pos.GetLine()) +
                                               ":" + std::to_string(pos.GetColumn())));
  }

  Advance();

  return {};
}

std::expected<void, BytecodeParserError> ParserContext::ExpectPunct(char ch, const std::string& msg) {
  if (!IsPunct(ch)) {
    auto token = Current();

    std::string out_msg = msg.empty() ? std::string("Expected '") + ch + "'" : msg;

    return std::unexpected(BytecodeParserError(out_msg + " at line " + std::to_string(token->GetPosition().GetLine()) +
                                               " column " + std::to_string(token->GetPosition().GetColumn())));
  }

  Advance();

  return {};
}

std::expected<std::string, BytecodeParserError> ParserContext::ConsumeIdentifier() {
  if (!IsIdentifier()) {
    auto token = Current();

    return std::unexpected(BytecodeParserError("Expected identifier at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  std::string value = Current()->GetLexeme();

  Advance();

  return value;
}

std::expected<std::string, BytecodeParserError> ParserContext::ConsumeStringLiteral() {
  if (Current()->GetStringType() != "LITERAL:String") {
    auto token = Current();

    return std::unexpected(BytecodeParserError("Expected string literal at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  std::string value = Current()->GetLexeme();
  value = value.substr(1, value.length() - 2);

  Advance();

  return value;
}

std::expected<int64_t, BytecodeParserError> ParserContext::ConsumeIntLiteral() {
  if (Current()->GetStringType() != "LITERAL:Int") {
    auto token = Current();

    return std::unexpected(BytecodeParserError("Expected integer literal at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  int64_t value = std::stoll(Current()->GetLexeme());

  Advance();

  return value;
}

std::expected<double, BytecodeParserError> ParserContext::ConsumeFloatLiteral() {
  if (Current()->GetStringType() != "LITERAL:Float") {
    auto token = Current();

    return std::unexpected(BytecodeParserError("Expected float literal at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  double value = std::stod(Current()->GetLexeme());

  Advance();

  return value;
}

std::expected<bool, BytecodeParserError> ParserContext::ConsumeBoolLiteral() {
  const std::string& lexeme = Current()->GetLexeme();

  if (lexeme == "true") {
    Advance();

    return true;
  }

  if (lexeme == "false") {
    Advance();

    return false;
  }

  auto token = Current();

  return std::unexpected(BytecodeParserError("Expected 'true' or 'false' at line " +
                                             std::to_string(token->GetPosition().GetLine()) + " column " +
                                             std::to_string(token->GetPosition().GetColumn())));
}

} // namespace ovum::bytecode::parser
