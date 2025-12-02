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

  const TokenPtr Current() const;
  bool IsEof() const;
  void Advance();

  bool IsIdentifier() const;
  bool IsKeyword(const std::string& kw) const;
  bool IsPunct(char ch) const;
  bool IsPunct(const std::string& p) const;

  std::expected<void, BytecodeParserError> ExpectKeyword(const std::string& kw);
  std::expected<void, BytecodeParserError> ExpectPunct(char ch, const std::string& msg = "");

  std::expected<std::string, BytecodeParserError> ConsumeIdentifier();
  std::expected<std::string, BytecodeParserError> ConsumeStringLiteral();
  std::expected<int64_t, BytecodeParserError> ConsumeIntLiteral();
  std::expected<double, BytecodeParserError> ConsumeFloatLiteral();
  std::expected<bool, BytecodeParserError> ConsumeBoolLiteral();

  vm::execution_tree::FunctionRepository& FuncRepo() const {
    return data_.func_repo;
  }
  vm::runtime::VirtualTableRepository& VTableRepo() const {
    return data_.vtable_repo;
  }
  vm::runtime::RuntimeMemory& Memory() const {
    return data_.memory;
  }

  vm::execution_tree::Block* CurrentBlock() const {
    return data_.current_block;
  }
  void SetCurrentBlock(vm::execution_tree::Block* block) {
    data_.current_block = block;
  }

  const std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>>& JitFactory() const {
    return data_.jit_factory;
  }
  size_t JitBoundary() const {
    return data_.jit_boundary;
  }

  std::unique_ptr<vm::execution_tree::Block> ReleaseInitStaticBlock() {
    return std::move(data_.init_static_block);
  }
  void SetInitStaticBlock(std::unique_ptr<vm::execution_tree::Block> block) {
    data_.init_static_block = std::move(block);
  }

private:
  const std::vector<TokenPtr>& tokens_;
  size_t pos_ = 0;
  ParsingSessionData& data_;
};

using ParsingSessionPtr = std::shared_ptr<ParsingSession>;

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_PARSINGSESSION_HPP_
