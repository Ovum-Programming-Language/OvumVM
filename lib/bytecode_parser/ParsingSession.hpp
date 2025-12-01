#ifndef BYTECODE_PARSER_PARSERCONTEXT_HPP_
#define BYTECODE_PARSER_PARSERCONTEXT_HPP_

#include <expected>
#include <string>
#include <vector>

#include <tokens/Token.hpp>

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

#include "BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

class ParsingSession {
public:
  explicit ParsingSession(const std::vector<TokenPtr>& tokens,
                         vm::execution_tree::FunctionRepository& func_repo,
                         vm::runtime::VirtualTableRepository& vtable_repo,
                         vm::runtime::RuntimeMemory& memory,
                         std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory,
                         size_t jit_boundary);

  [[nodiscard]] const TokenPtr Current() const;
  [[nodiscard]] bool IsEof() const;
  void Advance();

  [[nodiscard]] bool IsIdentifier() const;
  [[nodiscard]] bool IsKeyword(const std::string& kw) const;
  [[nodiscard]] bool IsPunct(char ch) const;
  [[nodiscard]] bool IsPunct(const std::string& p) const;

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
  vm::execution_tree::Block* current_block = nullptr;

  std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory;
  size_t jit_boundary = 0;

  std::unique_ptr<vm::execution_tree::Block> init_static_block;

private:
  const std::vector<TokenPtr>& tokens_;
  size_t pos_ = 0;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_PARSERCONTEXT_HPP_
