#ifndef BYTECODE_PARSER_BYTECODEPARSER_HPP_
#define BYTECODE_PARSER_BYTECODEPARSER_HPP_

#include <expected>
#include <memory>
#include <vector>

#include "BytecodeParserError.hpp"
#include "ParserContext.hpp"
#include "lib/bytecode_parser/scenarios/IParserHandler.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"

namespace ovum::bytecode::parser {

class BytecodeParser {
public:
  BytecodeParser(std::unique_ptr<vm::executor::IJitExecutorFactory> jit_factory, size_t jit_boundary);

  std::expected<void, BytecodeParserError> Parse(const std::vector<TokenPtr>& tokens,
                                                 vm::execution_tree::FunctionRepository& func_repo,
                                                 vm::runtime::VirtualTableRepository& vtable_repo,
                                                 vm::runtime::RuntimeMemory& memory);

private:
  std::vector<std::unique_ptr<IParserHandler>> handlers_;
  std::unique_ptr<vm::executor::IJitExecutorFactory> jit_factory_;
  size_t jit_boundary_;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_BYTECODEPARSER_HPP_
