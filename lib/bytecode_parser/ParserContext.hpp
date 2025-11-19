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
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::bytecode::parser {

class ParserContext {
public:
  explicit ParserContext(const std::vector<TokenPtr>& tokens,
                         vm::execution_tree::FunctionRepository& func_repo,
                         vm::runtime::VirtualTableRepository& vtable_repo,
                         vm::runtime::RuntimeMemory& memory,
                         vm::executor::IJitExecutorFactory* jit_factory,
                         size_t jit_boundary);

  [[nodiscard]] const TokenPtr Current() const;
  [[nodiscard]] bool IsEof() const;
  void Advance();

  [[nodiscard]] bool IsIdentifier() const;
  [[nodiscard]] bool IsKeyword(const std::string& kw) const;
  [[nodiscard]] bool IsPunct(char ch) const;
  [[nodiscard]] bool IsPunct(const std::string& p) const;
  [[nodiscard]] bool IsStringLiteral() const;
  [[nodiscard]] bool IsIntLiteral() const;
  [[nodiscard]] bool IsFloatLiteral() const;
  [[nodiscard]] bool IsBoolLiteral() const;

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

  vm::executor::IJitExecutorFactory* jit_factory = nullptr;
  size_t jit_boundary = 0;

private:
  const std::vector<TokenPtr>& tokens_;
  size_t pos_ = 0;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_PARSERCONTEXT_HPP_
