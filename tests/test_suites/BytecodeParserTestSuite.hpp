#ifndef BYTECODEPARSERTESTSUITE_HPP_
#define BYTECODEPARSERTESTSUITE_HPP_

#include <cstddef>
#include <expected>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "lib/bytecode_parser/BytecodeParser.hpp"
#include "lib/bytecode_parser/scenarios/PlaceholderCommandFactory.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/IFunctionExecutable.hpp"
#include "lib/executor/PlaceholderJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

struct BytecodeParserTestSuite : public testing::Test {
  static constexpr size_t kJitBoundary = 10;
  void SetUp() override;
  void TearDown() override;

  // Parser creation helpers
  ovum::bytecode::parser::BytecodeParser CreateParserWithJit(size_t jit_boundary = kJitBoundary);
  ovum::bytecode::parser::BytecodeParser CreateParserWithoutJit();

  // Token creation helpers
  std::vector<ovum::TokenPtr> TokenizeString(const std::string& input);

  // Parse helpers
  std::expected<std::unique_ptr<ovum::vm::execution_tree::Block>, ovum::bytecode::parser::BytecodeParserError>
  ParseSuccessfully(ovum::bytecode::parser::BytecodeParser& parser,
                    const std::vector<ovum::TokenPtr>& tokens,
                    ovum::vm::execution_tree::FunctionRepository& func_repo,
                    ovum::vm::runtime::VirtualTableRepository& vtable_repo);

  // Assertion helpers
  void AssertParseSuccess(ovum::bytecode::parser::BytecodeParser& parser,
                          const std::vector<ovum::TokenPtr>& tokens,
                          ovum::vm::execution_tree::FunctionRepository& func_repo,
                          ovum::vm::runtime::VirtualTableRepository& vtable_repo);
  void AssertParseError(ovum::bytecode::parser::BytecodeParser& parser,
                        const std::vector<ovum::TokenPtr>& tokens,
                        ovum::vm::execution_tree::FunctionRepository& func_repo,
                        ovum::vm::runtime::VirtualTableRepository& vtable_repo,
                        const std::string& error_substring);
  void AssertInitStaticBlockExists(const std::unique_ptr<ovum::vm::execution_tree::Block>& block);
  void AssertFunctionExists(ovum::vm::execution_tree::FunctionRepository& repo,
                            const std::string& name,
                            size_t expected_arity);
  void AssertFunctionCount(ovum::vm::execution_tree::FunctionRepository& repo, size_t expected_count);
  void AssertVtableExists(ovum::vm::runtime::VirtualTableRepository& repo, const std::string& name);
  void AssertVtableCount(ovum::vm::runtime::VirtualTableRepository& repo, size_t expected_count);
  void AssertFunctionType(ovum::vm::execution_tree::IFunctionExecutable* func, const std::string& expected_type);

private:
  std::unique_ptr<ovum::vm::executor::PlaceholderJitExecutorFactory> jit_factory_;
  ovum::bytecode::parser::PlaceholderCommandFactory command_factory_;
  ovum::vm::runtime::RuntimeMemory memory_;
};

#endif // BYTECODEPARSERTESTSUITE_HPP_
