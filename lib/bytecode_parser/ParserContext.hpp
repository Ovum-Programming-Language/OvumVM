#ifndef BYTECODE_PARSER_PARSERCONTEXT_HPP_
#define BYTECODE_PARSER_PARSERCONTEXT_HPP_

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include <tokens/Token.hpp>
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::bytecode::parser {

class ParserContext {
public:
  ParserContext(const std::vector<TokenPtr>& tokens,
                vm::execution_tree::FunctionRepository& func_repo,
                vm::runtime::VirtualTableRepository& vtable_repo,
                vm::runtime::RuntimeMemory& memory);

  const TokenPtr Current() const;
  const TokenPtr Peek(size_t offset = 1) const;
  bool IsEof() const;
  void Advance();

  bool IsIdentifier() const;
  bool IsKeyword(const std::string& kw) const;
  bool IsPunct(char ch) const;
  bool IsPunct(const std::string& p) const;
  bool IsStringLiteral() const;
  bool IsIntLiteral() const;
  bool IsFloatLiteral() const;
  bool IsBoolLiteral() const;

  std::expected<void, BytecodeParserError> ExpectIdentifier(const std::string& msg);
  std::expected<void, BytecodeParserError> ExpectKeyword(const std::string& kw);
  std::expected<void, BytecodeParserError> ExpectPunct(char ch, const std::string& msg = "");

  std::expected<std::string, BytecodeParserError> ConsumeIdentifier();
  std::expected<std::string, BytecodeParserError> ConsumeStringLiteral();
  std::expected<int64_t, BytecodeParserError> ConsumeIntLiteral();
  std::expected<double, BytecodeParserError> ConsumeFloatLiteral();
  std::expected<bool, BytecodeParserError> ConsumeBoolLiteral();

  vm::execution_tree::FunctionRepository& func_repo;
  vm::runtime::VirtualTableRepository& vtable_repo;
  vm::runtime::RuntimeMemory& memory;

  bool init_static_parsed = false;

  ovum::vm::execution_tree::Block* current_block = nullptr;

private:
  const std::vector<TokenPtr>& tokens_;
  size_t pos_ = 0;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_PARSERCONTEXT_HPP_
