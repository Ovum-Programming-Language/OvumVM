#include "BytecodeParserTestSuite.hpp"

#include "lib/bytecode_lexer/BytecodeLexer.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/JitCompilingFunction.hpp"
#include "lib/execution_tree/PureFunction.hpp"

void BytecodeParserTestSuite::SetUp() {
  jit_factory_ = std::make_unique<ovum::vm::executor::PlaceholderJitExecutorFactory>();
}

void BytecodeParserTestSuite::TearDown() {
  jit_factory_.reset();
}

ovum::bytecode::parser::BytecodeParser BytecodeParserTestSuite::CreateParserWithJit(size_t jit_boundary) {
  return ovum::bytecode::parser::BytecodeParser(
      std::make_unique<ovum::vm::executor::PlaceholderJitExecutorFactory>(), jit_boundary, command_factory_);
}

ovum::bytecode::parser::BytecodeParser BytecodeParserTestSuite::CreateParserWithoutJit() {
  return ovum::bytecode::parser::BytecodeParser(nullptr, 0, command_factory_);
}

std::vector<ovum::TokenPtr> BytecodeParserTestSuite::TokenizeString(const std::string& input) {
  ovum::bytecode::lexer::BytecodeLexer lexer(input);
  auto result = lexer.Tokenize();
  EXPECT_TRUE(result.has_value()) << "Tokenization should succeed for: " << input;
  if (!result.has_value()) {
    return {};
  }
  return result.value();
}

std::expected<std::unique_ptr<ovum::vm::execution_tree::Block>, ovum::bytecode::parser::BytecodeParserError>
BytecodeParserTestSuite::ParseSuccessfully(ovum::bytecode::parser::BytecodeParser& parser,
                                           const std::vector<ovum::TokenPtr>& tokens,
                                           ovum::vm::execution_tree::FunctionRepository& func_repo,
                                           ovum::vm::runtime::VirtualTableRepository& vtable_repo) {
  auto result = parser.Parse(tokens, func_repo, vtable_repo, memory_);
  if (!result.has_value()) {
    std::string error_msg = result.error().what();
    EXPECT_TRUE(result.has_value()) << "Parse should succeed. Error: " << error_msg;
  }
  return result;
}

void BytecodeParserTestSuite::AssertParseSuccess(ovum::bytecode::parser::BytecodeParser& parser,
                                                 const std::vector<ovum::TokenPtr>& tokens,
                                                 ovum::vm::execution_tree::FunctionRepository& func_repo,
                                                 ovum::vm::runtime::VirtualTableRepository& vtable_repo) {
  auto result = parser.Parse(tokens, func_repo, vtable_repo, memory_);
  ASSERT_TRUE(result.has_value()) << "Parse should succeed";
}

void BytecodeParserTestSuite::AssertParseError(ovum::bytecode::parser::BytecodeParser& parser,
                                               const std::vector<ovum::TokenPtr>& tokens,
                                               ovum::vm::execution_tree::FunctionRepository& func_repo,
                                               ovum::vm::runtime::VirtualTableRepository& vtable_repo,
                                               const std::string& error_substring) {
  auto result = parser.Parse(tokens, func_repo, vtable_repo, memory_);
  ASSERT_FALSE(result.has_value()) << "Parse should fail";
  std::string error_msg = result.error().what();
  ASSERT_NE(error_msg.find(error_substring), std::string::npos)
      << "Error message should contain '" << error_substring << "', but got: " << error_msg;
}

void BytecodeParserTestSuite::AssertInitStaticBlockExists(
    const std::unique_ptr<ovum::vm::execution_tree::Block>& block) {
  ASSERT_NE(block, nullptr) << "Init-static block should exist";
}

void BytecodeParserTestSuite::AssertFunctionExists(ovum::vm::execution_tree::FunctionRepository& repo,
                                                   const std::string& name,
                                                   size_t expected_arity) {
  auto func_result = repo.GetByName(name);
  ASSERT_TRUE(func_result.has_value()) << "Function '" << name << "' should exist";
  ASSERT_EQ(func_result.value()->GetArity(), expected_arity)
      << "Function '" << name << "' should have arity " << expected_arity;
}

void BytecodeParserTestSuite::AssertFunctionCount(ovum::vm::execution_tree::FunctionRepository& repo,
                                                  size_t expected_count) {
  ASSERT_EQ(repo.GetCount(), expected_count) << "Function repository should have " << expected_count << " functions";
}

void BytecodeParserTestSuite::AssertVtableExists(ovum::vm::runtime::VirtualTableRepository& repo,
                                                 const std::string& name) {
  auto vtable_result = repo.GetByName(name);
  ASSERT_TRUE(vtable_result.has_value()) << "Vtable '" << name << "' should exist";
}

void BytecodeParserTestSuite::AssertVtableCount(ovum::vm::runtime::VirtualTableRepository& repo,
                                                size_t expected_count) {
  ASSERT_EQ(repo.GetCount(), expected_count) << "Vtable repository should have " << expected_count << " vtables";
}

void BytecodeParserTestSuite::AssertFunctionType(ovum::vm::execution_tree::IFunctionExecutable* func,
                                                 const std::string& expected_type) {
  ASSERT_NE(func, nullptr) << "Function should not be null";

  // Use dynamic_cast to check the actual type (avoid typeid which can cause issues)
  if (expected_type == "RegularFunction") {
    // Check if it's a Function (not wrapped)
    bool is_regular =
        dynamic_cast<ovum::vm::execution_tree::Function*>(func) != nullptr &&
        dynamic_cast<ovum::vm::execution_tree::JitCompilingFunction<ovum::vm::execution_tree::Function>*>(func) ==
            nullptr &&
        dynamic_cast<ovum::vm::execution_tree::PureFunction<ovum::vm::execution_tree::Function>*>(func) == nullptr;
    ASSERT_TRUE(is_regular) << "Function should be RegularFunction";
  } else if (expected_type == "JitFunction") {
    // Check if it's a JitCompilingFunction<Function>
    bool is_jit =
        dynamic_cast<ovum::vm::execution_tree::JitCompilingFunction<ovum::vm::execution_tree::Function>*>(func) !=
            nullptr &&
        dynamic_cast<ovum::vm::execution_tree::PureFunction<
            ovum::vm::execution_tree::JitCompilingFunction<ovum::vm::execution_tree::Function>>*>(func) == nullptr;
    ASSERT_TRUE(is_jit) << "Function should be JitFunction";
  } else if (expected_type == "PureFunction") {
    // Check if it's a PureFunction<Function>
    bool is_pure =
        dynamic_cast<ovum::vm::execution_tree::PureFunction<ovum::vm::execution_tree::Function>*>(func) != nullptr &&
        dynamic_cast<ovum::vm::execution_tree::JitCompilingFunction<ovum::vm::execution_tree::Function>*>(func) ==
            nullptr;
    ASSERT_TRUE(is_pure) << "Function should be PureFunction";
  } else if (expected_type == "PureJitFunction") {
    // Check if it's a PureFunction<JitCompilingFunction<Function>>
    bool is_pure_jit =
        dynamic_cast<ovum::vm::execution_tree::PureFunction<
            ovum::vm::execution_tree::JitCompilingFunction<ovum::vm::execution_tree::Function>>*>(func) != nullptr;
    ASSERT_TRUE(is_pure_jit) << "Function should be PureJitFunction";
  } else {
    FAIL() << "Unknown expected function type: " << expected_type;
  }
}
