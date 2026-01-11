#ifndef BUILTINTESTSUITE_HPP_
#define BUILTINTESTSUITE_HPP_

#include <cstdint>
#include <expected>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/IExecutable.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"
#include "lib/runtime/gc/MarkAndSweepGC.hpp"

struct BuiltinTestSuite : public testing::Test {
  BuiltinTestSuite();

  void SetUp() override;
  void TearDown() override;

  // Command creation helpers
  std::unique_ptr<ovum::vm::execution_tree::IExecutable> MakeSimple(const std::string& name);
  std::unique_ptr<ovum::vm::execution_tree::IExecutable> MakeStringCmd(const std::string& name, const std::string& arg);
  std::unique_ptr<ovum::vm::execution_tree::IExecutable> MakeIntCmd(const std::string& name, int64_t arg);
  std::unique_ptr<ovum::vm::execution_tree::IExecutable> MakeFloatCmd(const std::string& name, double arg);
  std::unique_ptr<ovum::vm::execution_tree::IExecutable> MakeBoolCmd(const std::string& name, bool arg);

  // Object helpers
  void* MakeString(const std::string& value);
  void* MakeNullable(void* wrapped);
  void* MakeStringArray(const std::vector<std::string>& values);
  void* MakeByteArray(const std::vector<uint8_t>& values);

  // Stack helpers
  void PushInt(int64_t value);
  void PushFloat(double value);
  void PushBool(bool value);
  void PushChar(char value);
  void PushByte(uint8_t value);
  void PushObject(void* ptr);

  int64_t PopInt();
  double PopDouble();
  bool PopBool();
  char PopChar();
  uint8_t PopByte();
  void* PopObject();

  // Verification helpers
  void ExpectTopStringEquals(const std::string& expected);
  void ExpectTopNullableHasValue(bool has_value);

  // Memory cleanup
  void CleanupObjects();
  void DestroyObject(void* obj);

  constexpr static size_t kDefaultMaxObjects = 10000;

  // Repositories for fixtures
  ovum::vm::runtime::RuntimeMemory memory_{};
  ovum::vm::runtime::VirtualTableRepository vtable_repo_{};
  ovum::vm::execution_tree::FunctionRepository function_repo_{};
  std::stringstream input_stream_;
  std::stringstream output_stream_;
  std::stringstream error_stream_;
  ovum::vm::runtime::MemoryManager memory_manager_{std::make_unique<ovum::vm::runtime::MarkAndSweepGC>(),
                                                   kDefaultMaxObjects};
  ovum::vm::execution_tree::PassedExecutionData data_;
};

#endif // BUILTINTESTSUITE_HPP_
