#ifndef BYTECODE_PARSER_PARSINGSESSION_HPP_
#define BYTECODE_PARSER_PARSINGSESSION_HPP_

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include <tokens/Token.hpp>

#include "BytecodeParserError.hpp"
#include "ParsingSessionData.hpp"

namespace ovum::bytecode::parser {

class ParsingSession {
public:
  ParsingSession(const std::vector<TokenPtr>& tokens, ParsingSessionData& data);

  [[nodiscard]] const TokenPtr Current() const;
  [[nodiscard]] bool IsEof() const;
  void Advance();

  [[nodiscard]] bool IsIdentifier() const;
  [[nodiscard]] bool IsKeyword(const std::string& kw) const;
  [[nodiscard]] bool IsPunct(char ch) const;

  std::expected<void, BytecodeParserError> ExpectKeyword(const std::string& kw);
  std::expected<void, BytecodeParserError> ExpectPunct(char ch, const std::string& msg = "");

  std::expected<std::string, BytecodeParserError> ConsumeIdentifier();
  std::expected<std::string, BytecodeParserError> ConsumeStringLiteral();
  std::expected<int64_t, BytecodeParserError> ConsumeIntLiteral();
  std::expected<double, BytecodeParserError> ConsumeFloatLiteral();
  std::expected<bool, BytecodeParserError> ConsumeBoolLiteral();

  [[nodiscard]] vm::execution_tree::FunctionRepository& GetFuncRepo() const;
  [[nodiscard]] vm::runtime::VirtualTableRepository& GetVTableRepo() const;

  [[nodiscard]] vm::execution_tree::Block* GetCurrentBlock() const;
  void SetCurrentBlock(vm::execution_tree::Block* block);

  [[nodiscard]] const std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>>& GetJitFactory() const;
  [[nodiscard]] size_t GetJitBoundary() const;

  std::unique_ptr<vm::execution_tree::Block> GetInitStaticBlock();
  void SetInitStaticBlock(std::unique_ptr<vm::execution_tree::Block> block);

  std::vector<TokenPtr> CopyUntilBlockEnd();

private:
  const std::vector<TokenPtr>& tokens_;
  size_t pos_ = 0;
  ParsingSessionData& data_;
};

using ParsingSessionPtr = std::shared_ptr<ParsingSession>;

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_PARSINGSESSION_HPP_
