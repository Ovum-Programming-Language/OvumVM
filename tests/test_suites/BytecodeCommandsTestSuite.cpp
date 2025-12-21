#include "BytecodeCommandsTestSuite.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>

#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/command_factory.hpp"
#include "lib/executor/BuiltinFunctions.hpp"
#include "lib/executor/builtin_factory.hpp"
#include "lib/runtime/ByteArray.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"

using ovum::vm::execution_tree::CreateBooleanCommandByName;
using ovum::vm::execution_tree::CreateFloatCommandByName;
using ovum::vm::execution_tree::CreateIntegerCommandByName;
using ovum::vm::execution_tree::CreateSimpleCommandByName;
using ovum::vm::execution_tree::CreateStringCommandByName;
using ovum::vm::execution_tree::ExecutionResult;
using ovum::vm::execution_tree::Function;
using ovum::vm::execution_tree::IExecutable;
using ovum::vm::execution_tree::PassedExecutionData;
using ovum::vm::runtime::AllocateObject;
using ovum::vm::runtime::GetDataPointer;
using ovum::vm::runtime::ObjectDescriptor;
using ovum::vm::runtime::StackFrame;
using ovum::vm::runtime::Variable;

BytecodeCommandsTestSuite::BytecodeCommandsTestSuite() :
    data_(memory_, vtable_repo_, function_repo_, allocator_, input_stream_, output_stream_, error_stream_) {
}

void BytecodeCommandsTestSuite::SetUp() {
  auto vtable_result = ovum::vm::runtime::RegisterBuiltinVirtualTables(vtable_repo_);
  ASSERT_TRUE(vtable_result.has_value()) << vtable_result.error().what();

  auto function_result = ovum::vm::execution_tree::RegisterBuiltinFunctions(function_repo_);
  ASSERT_TRUE(function_result.has_value()) << function_result.error().what();

  StackFrame frame{};
  frame.function_name = "test";
  memory_.stack_frames.push(std::move(frame));
}

void BytecodeCommandsTestSuite::TearDown() {
  CleanupObjects();
  while (!memory_.machine_stack.empty()) {
    memory_.machine_stack.pop();
  }
  while (!memory_.stack_frames.empty()) {
    memory_.stack_frames.pop();
  }
}

std::unique_ptr<IExecutable> BytecodeCommandsTestSuite::MakeSimple(const std::string& name) {
  auto cmd = CreateSimpleCommandByName(name);
  EXPECT_TRUE(cmd.has_value()) << "Simple command not found: " << name;
  if (!cmd.has_value()) {
    return nullptr;
  }
  return std::move(cmd.value());
}

std::unique_ptr<IExecutable> BytecodeCommandsTestSuite::MakeStringCmd(const std::string& name, const std::string& arg) {
  auto cmd = CreateStringCommandByName(name, arg);
  EXPECT_TRUE(cmd.has_value()) << "String command not found: " << name;
  if (!cmd.has_value()) {
    return nullptr;
  }
  return std::move(cmd.value());
}

std::unique_ptr<IExecutable> BytecodeCommandsTestSuite::MakeIntCmd(const std::string& name, int64_t arg) {
  auto cmd = CreateIntegerCommandByName(name, arg);
  EXPECT_TRUE(cmd.has_value()) << "Integer command not found: " << name;
  if (!cmd.has_value()) {
    return nullptr;
  }
  return std::move(cmd.value());
}

std::unique_ptr<IExecutable> BytecodeCommandsTestSuite::MakeFloatCmd(const std::string& name, double arg) {
  auto cmd = CreateFloatCommandByName(name, arg);
  EXPECT_TRUE(cmd.has_value()) << "Float command not found: " << name;
  if (!cmd.has_value()) {
    return nullptr;
  }
  return std::move(cmd.value());
}

std::unique_ptr<IExecutable> BytecodeCommandsTestSuite::MakeBoolCmd(const std::string& name, bool arg) {
  auto cmd = CreateBooleanCommandByName(name, arg);
  EXPECT_TRUE(cmd.has_value()) << "Boolean command not found: " << name;
  if (!cmd.has_value()) {
    return nullptr;
  }
  return std::move(cmd.value());
}

void* BytecodeCommandsTestSuite::MakeString(const std::string& value) {
  auto vt = vtable_repo_.GetByName("String");
  if (!vt.has_value()) {
    return nullptr;
  }
  auto idx = vtable_repo_.GetIndexByName("String");
  if (!idx.has_value()) {
    return nullptr;
  }
  auto obj_result =
      AllocateObject(*vt.value(), static_cast<uint32_t>(idx.value()), memory_.object_repository, data_.allocator);
  if (!obj_result.has_value()) {
    return nullptr;
  }
  void* obj = obj_result.value();
  auto* str_ptr = GetDataPointer<std::string>(obj);
  new (str_ptr) std::string(value);
  return obj;
}

void* BytecodeCommandsTestSuite::MakeNullable(void* wrapped) {
  auto vt = vtable_repo_.GetByName("Nullable");
  if (!vt.has_value()) {
    return nullptr;
  }
  auto idx = vtable_repo_.GetIndexByName("Nullable");
  if (!idx.has_value()) {
    return nullptr;
  }
  auto obj_result =
      AllocateObject(*vt.value(), static_cast<uint32_t>(idx.value()), memory_.object_repository, data_.allocator);
  if (!obj_result.has_value()) {
    return nullptr;
  }

  void* obj = obj_result.value();
  auto* nullable_ptr = GetDataPointer<void*>(obj);
  *nullable_ptr = wrapped;
  return obj;
}

void* BytecodeCommandsTestSuite::MakeStringArray(const std::vector<std::string>& values) {
  auto vt = vtable_repo_.GetByName("StringArray");
  if (!vt.has_value()) {
    return nullptr;
  }
  auto idx = vtable_repo_.GetIndexByName("StringArray");
  if (!idx.has_value()) {
    return nullptr;
  }

  auto obj_result =
      AllocateObject(*vt.value(), static_cast<uint32_t>(idx.value()), memory_.object_repository, data_.allocator);
  if (!obj_result.has_value()) {
    return nullptr;
  }
  void* obj = obj_result.value();
  auto* vec_ptr = GetDataPointer<std::vector<void*>>(obj);
  new (vec_ptr) std::vector<void*>();
  for (const auto& val : values) {
    vec_ptr->push_back(MakeString(val));
  }
  return obj;
}

void* BytecodeCommandsTestSuite::MakeByteArray(const std::vector<uint8_t>& values) {
  auto vt = vtable_repo_.GetByName("ByteArray");
  if (!vt.has_value()) {
    return nullptr;
  }
  auto idx = vtable_repo_.GetIndexByName("ByteArray");
  if (!idx.has_value()) {
    return nullptr;
  }

  auto obj_result =
      AllocateObject(*vt.value(), static_cast<uint32_t>(idx.value()), memory_.object_repository, data_.allocator);
  if (!obj_result.has_value()) {
    return nullptr;
  }

  void* obj = obj_result.value();
  auto* byte_array = GetDataPointer<ovum::vm::runtime::ByteArray>(obj);
  new (byte_array) ovum::vm::runtime::ByteArray(values.size());
  if (values.size() > 0) {
    std::copy(values.begin(), values.end(), byte_array->Data());
  }
  return obj;
}

void BytecodeCommandsTestSuite::PushInt(int64_t value) {
  memory_.machine_stack.emplace(value);
}

void BytecodeCommandsTestSuite::PushFloat(double value) {
  memory_.machine_stack.emplace(value);
}

void BytecodeCommandsTestSuite::PushBool(bool value) {
  memory_.machine_stack.emplace(value);
}

void BytecodeCommandsTestSuite::PushChar(char value) {
  memory_.machine_stack.emplace(value);
}

void BytecodeCommandsTestSuite::PushByte(uint8_t value) {
  memory_.machine_stack.emplace(value);
}

void BytecodeCommandsTestSuite::PushObject(void* ptr) {
  memory_.machine_stack.emplace(ptr);
}

int64_t BytecodeCommandsTestSuite::PopInt() {
  auto var = memory_.machine_stack.top();
  memory_.machine_stack.pop();
  return std::get<int64_t>(var);
}

double BytecodeCommandsTestSuite::PopDouble() {
  auto var = memory_.machine_stack.top();
  memory_.machine_stack.pop();
  return std::get<double>(var);
}

bool BytecodeCommandsTestSuite::PopBool() {
  auto var = memory_.machine_stack.top();
  memory_.machine_stack.pop();
  return std::get<bool>(var);
}

char BytecodeCommandsTestSuite::PopChar() {
  auto var = memory_.machine_stack.top();
  memory_.machine_stack.pop();
  return std::get<char>(var);
}

uint8_t BytecodeCommandsTestSuite::PopByte() {
  auto var = memory_.machine_stack.top();
  memory_.machine_stack.pop();
  return std::get<uint8_t>(var);
}

void* BytecodeCommandsTestSuite::PopObject() {
  auto var = memory_.machine_stack.top();
  memory_.machine_stack.pop();
  return std::get<void*>(var);
}

void BytecodeCommandsTestSuite::ExpectTopStringEquals(const std::string& expected) {
  ASSERT_FALSE(memory_.machine_stack.empty());
  auto var = memory_.machine_stack.top();
  ASSERT_TRUE(std::holds_alternative<void*>(var));
  auto* str_ptr = GetDataPointer<std::string>(std::get<void*>(var));
  EXPECT_EQ(*str_ptr, expected);
}

void BytecodeCommandsTestSuite::ExpectTopNullableHasValue(bool has_value) {
  ASSERT_FALSE(memory_.machine_stack.empty());
  auto var = memory_.machine_stack.top();
  ASSERT_TRUE(std::holds_alternative<void*>(var));
  auto* nullable_ptr = GetDataPointer<void*>(std::get<void*>(var));
  if (has_value) {
    EXPECT_NE(*nullable_ptr, nullptr);
  } else {
    EXPECT_EQ(*nullable_ptr, nullptr);
  }
}

void BytecodeCommandsTestSuite::CleanupObjects() {
  std::vector<void*> objects;
  for (size_t i = 0; i < memory_.object_repository.GetCount(); ++i) {
    auto obj = memory_.object_repository.GetByIndex(i);
    if (obj.has_value()) {
      objects.push_back(obj.value());
    }
  }

  for (void* obj : objects) {
    DestroyObject(obj);
  }

  memory_.object_repository.Clear();
}

void BytecodeCommandsTestSuite::DestroyObject(void* obj) {
  auto* descriptor = reinterpret_cast<ObjectDescriptor*>(obj);
  auto vt = vtable_repo_.GetByIndex(descriptor->vtable_index);
  if (!vt.has_value()) {
    return;
  }

  const std::string name = vt.value()->GetName();
  if (name == "String") {
    auto* ptr = GetDataPointer<std::string>(obj);
    ptr->~basic_string();
  } else if (name == "StringArray" || name == "ObjectArray" || name == "PointerArray") {
    auto* ptr = GetDataPointer<std::vector<void*>>(obj);
    ptr->~vector();
  } else if (name == "IntArray") {
    auto* ptr = GetDataPointer<std::vector<int64_t>>(obj);
    ptr->~vector();
  } else if (name == "FloatArray") {
    auto* ptr = GetDataPointer<std::vector<double>>(obj);
    ptr->~vector();
  } else if (name == "CharArray") {
    auto* ptr = GetDataPointer<std::vector<char>>(obj);
    ptr->~vector();
  } else if (name == "ByteArray") {
    auto* ptr = GetDataPointer<ovum::vm::runtime::ByteArray>(obj);
    ptr->~ByteArray();
  } else if (name == "BoolArray") {
    auto* ptr = GetDataPointer<std::vector<bool>>(obj);
    ptr->~vector();
  } else if (name == "File") {
    auto* ptr = GetDataPointer<std::fstream>(obj);
    ptr->~basic_fstream();
  }

  allocator_.deallocate(reinterpret_cast<char*>(obj), vt.value()->GetSize());
}
