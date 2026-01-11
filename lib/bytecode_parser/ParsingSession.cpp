#include "ParsingSession.hpp"

#include <tokens/EofToken.hpp>
#include <tokens/LiteralToken.hpp>
#include <tokens/values/StringValue.hpp>

namespace ovum::bytecode::parser {

vm::execution_tree::FunctionRepository& ParsingSession::GetFuncRepo() const {
  return data_.func_repo;
}

vm::runtime::VirtualTableRepository& ParsingSession::GetVTableRepo() const {
  return data_.vtable_repo;
}

vm::execution_tree::Block* ParsingSession::GetCurrentBlock() const {
  return data_.current_block;
}
void ParsingSession::SetCurrentBlock(vm::execution_tree::Block* block) {
  data_.current_block = block;
}

const std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>>& ParsingSession::GetJitFactory() const {
  return data_.jit_factory;
}
[[nodiscard]] size_t ParsingSession::GetJitBoundary() const {
  return data_.jit_boundary;
}

std::unique_ptr<vm::execution_tree::Block> ParsingSession::GetInitStaticBlock() {
  return std::move(data_.init_static_block);
}
void ParsingSession::SetInitStaticBlock(std::unique_ptr<vm::execution_tree::Block> block) {
  data_.init_static_block = std::move(block);
}

ParsingSession::ParsingSession(const std::vector<TokenPtr>& tokens, ParsingSessionData& data) :
    tokens_(tokens), data_(data) {
}

const TokenPtr ParsingSession::Current() const {
  if (pos_ >= tokens_.size()) {
    static const EofToken kEof(TokenPosition(0, 0));
    return std::make_shared<EofToken>(kEof);
  }
  return tokens_[pos_];
}

bool ParsingSession::IsEof() const {
  return pos_ >= tokens_.size() || tokens_[pos_]->GetStringType() == "EOF";
}

void ParsingSession::Advance() {
  if (!IsEof())
    ++pos_;
}

bool ParsingSession::IsIdentifier() const {
  return Current()->GetStringType() == "IDENT";
}

bool ParsingSession::IsKeyword(const std::string& kw) const {
  return Current()->GetStringType() == "KEYWORD" && Current()->GetLexeme() == kw;
}

bool ParsingSession::IsPunct(char ch) const {
  return Current()->GetStringType() == "PUNCT" && Current()->GetLexeme() == std::string(1, ch);
}

std::expected<void, BytecodeParserError> ParsingSession::ExpectKeyword(const std::string& kw) {
  if (!IsKeyword(kw)) {
    TokenPosition pos = Current()->GetPosition();

    return std::unexpected(BytecodeParserError("Expected keyword '" + kw + "' at " + std::to_string(pos.GetLine()) +
                                               ":" + std::to_string(pos.GetColumn())));
  }

  Advance();

  return {};
}

std::expected<void, BytecodeParserError> ParsingSession::ExpectPunct(char ch, const std::string& msg) {
  if (!IsPunct(ch)) {
    TokenPtr token = Current();
    std::string out_msg = msg.empty() ? std::string("Expected '") + ch + "'" : msg;

    return std::unexpected(BytecodeParserError(out_msg + " at line " + std::to_string(token->GetPosition().GetLine()) +
                                               " column " + std::to_string(token->GetPosition().GetColumn())));
  }

  Advance();

  return {};
}

std::expected<std::string, BytecodeParserError> ParsingSession::ConsumeIdentifier() {
  if (!IsIdentifier()) {
    TokenPtr token = Current();

    return std::unexpected(BytecodeParserError("Expected identifier at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  std::string value = Current()->GetLexeme();

  Advance();

  return value;
}

std::expected<std::string, BytecodeParserError> ParsingSession::ConsumeStringLiteral() {
  if (Current()->GetStringType() != "LITERAL:String") {
    TokenPtr token = Current();

    return std::unexpected(BytecodeParserError("Expected string literal at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  std::string value =
      dynamic_cast<StringValue*>(dynamic_cast<ovum::LiteralToken*>(Current().get())->GetValue())->ToString();

  value = value.substr(1, value.length() - 2);

  Advance();

  return value;
}

std::expected<int64_t, BytecodeParserError> ParsingSession::ConsumeIntLiteral() {
  if (Current()->GetStringType() != "LITERAL:Int") {
    TokenPtr token = Current();

    return std::unexpected(BytecodeParserError("Expected integer literal at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  int64_t value = std::stoll(Current()->GetLexeme());

  Advance();

  return value;
}

std::expected<double, BytecodeParserError> ParsingSession::ConsumeFloatLiteral() {
  if (Current()->GetStringType() != "LITERAL:Float") {
    TokenPtr token = Current();

    return std::unexpected(BytecodeParserError("Expected float literal at line " +
                                               std::to_string(token->GetPosition().GetLine()) + " column " +
                                               std::to_string(token->GetPosition().GetColumn())));
  }

  double value = std::stod(Current()->GetLexeme());

  Advance();

  return value;
}

std::expected<bool, BytecodeParserError> ParsingSession::ConsumeBoolLiteral() {
  const std::string& lexeme = Current()->GetLexeme();

  if (lexeme == "true") {
    Advance();

    return true;
  }

  if (lexeme == "false") {
    Advance();

    return false;
  }

  TokenPtr token = Current();

  return std::unexpected(BytecodeParserError("Expected 'true' or 'false' at line " +
                                             std::to_string(token->GetPosition().GetLine()) + " column " +
                                             std::to_string(token->GetPosition().GetColumn())));
}


std::vector<TokenPtr> ParsingSession::CopyUntilBlockEnd() {
  std::vector<TokenPtr> result;
  size_t pos = pos_; 
  size_t cnt = 1;
  while (pos < tokens_.size() && tokens_[pos]->GetStringType() != "EOF") {
    if (tokens_[pos]->GetStringType() == "PUNCT" && tokens_[pos]->GetLexeme() == std::string(1, '}')) {
      --cnt;
      if (cnt == 0) {
        break;
      }
    } else if (tokens_[pos]->GetStringType() == "PUNCT" && tokens_[pos]->GetLexeme() == std::string(1, '{')) {
      ++cnt;
    }

    result.push_back(tokens_[pos]);
    ++pos;
  }

  return result;
}

} // namespace ovum::bytecode::parser
