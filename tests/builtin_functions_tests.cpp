#include "test_suites/BuiltinTestSuite.hpp"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/executor/BuiltinFunctions.hpp"
#include "lib/runtime/ByteArray.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/Variable.hpp"

namespace {

void* AllocateObjectByName(BuiltinTestSuite& suite, const std::string& vtable_name) {
  auto vt = suite.vtable_repo_.GetByName(vtable_name);
  EXPECT_TRUE(vt.has_value()) << "Missing vtable: " << vtable_name;
  if (!vt.has_value()) {
    return nullptr;
  }
  auto idx = suite.vtable_repo_.GetIndexByName(vtable_name);
  EXPECT_TRUE(idx.has_value()) << "Missing vtable index: " << vtable_name;
  if (!idx.has_value()) {
    return nullptr;
  }
  auto obj_result = suite.memory_manager_.AllocateObject(
      *vt.value(), static_cast<uint32_t>(idx.value()), suite.data_);
  EXPECT_TRUE(obj_result.has_value()) << "Allocation failed for " << vtable_name;
  if (!obj_result.has_value()) {
    return nullptr;
  }
  return obj_result.value();
}

template<typename... Args>
std::expected<ovum::vm::execution_tree::ExecutionResult, std::runtime_error> ExecuteFunction(BuiltinTestSuite& suite,
                                                                                             const std::string& name,
                                                                                             Args&&... args) {
  auto fn = suite.function_repo_.GetByName(name);
  EXPECT_TRUE(fn.has_value()) << "Function not found: " << name;
  if (!fn.has_value()) {
    return std::unexpected(std::runtime_error("Function missing: " + name));
  }

  auto tuple_args = std::make_tuple(std::forward<Args>(args)...);
  constexpr size_t kCount = sizeof...(Args);
  if constexpr (kCount > 0) {
    auto push_reversed = [&]<size_t... Is>(std::index_sequence<Is...>) {
      (suite.memory_.machine_stack.emplace(std::get<kCount - 1U - Is>(tuple_args)), ...);
    };
    push_reversed(std::make_index_sequence<kCount>{});
  }

  return fn.value()->Execute(suite.data_);
}

template<typename T>
void ExpectStackTopEquals(BuiltinTestSuite& suite, const T& expected) {
  ASSERT_FALSE(suite.memory_.machine_stack.empty());
  auto var = suite.memory_.machine_stack.top();
  suite.memory_.machine_stack.pop();
  ASSERT_TRUE(std::holds_alternative<T>(var));
  EXPECT_EQ(std::get<T>(var), expected);
}

void ExpectStackTopPointer(BuiltinTestSuite& suite, void* expected) {
  ASSERT_FALSE(suite.memory_.machine_stack.empty());
  auto var = suite.memory_.machine_stack.top();
  suite.memory_.machine_stack.pop();
  ASSERT_TRUE(std::holds_alternative<void*>(var));
  EXPECT_EQ(std::get<void*>(var), expected);
}

} // namespace

TEST_F(BuiltinTestSuite, PrimitiveIntMethods) {
  constexpr int64_t kValue = 42;
  constexpr int64_t kCopyValue = -7;
  constexpr int64_t kLessLeft = 1;
  constexpr int64_t kLessRight = 3;

  void* int_obj = AllocateObjectByName(*this, "Int");
  ASSERT_NE(int_obj, nullptr);
  auto ctor = ExecuteFunction(*this, "_Int_int", int_obj, kValue);
  ASSERT_TRUE(ctor.has_value());
  ExpectStackTopPointer(*this, int_obj);
  auto* int_data = ovum::vm::runtime::GetDataPointer<int64_t>(int_obj);
  EXPECT_EQ(*int_data, kValue);

  void* copy_src = AllocateObjectByName(*this, "Int");
  ASSERT_NE(copy_src, nullptr);
  *ovum::vm::runtime::GetDataPointer<int64_t>(copy_src) = kCopyValue;
  void* copy_dst = AllocateObjectByName(*this, "Int");
  ASSERT_NE(copy_dst, nullptr);
  auto copy = ExecuteFunction(*this, "_Int_Int", copy_dst, copy_src);
  ASSERT_TRUE(copy.has_value());
  ExpectStackTopPointer(*this, copy_dst);
  EXPECT_EQ(*ovum::vm::runtime::GetDataPointer<int64_t>(copy_dst), kCopyValue);

  auto equals_true = ExecuteFunction(*this, "_Int_Equals_<C>_Object", copy_dst, copy_src);
  ASSERT_TRUE(equals_true.has_value());
  ExpectStackTopEquals<bool>(*this, true);

  void* other_type = AllocateObjectByName(*this, "Float");
  ASSERT_NE(other_type, nullptr);
  *ovum::vm::runtime::GetDataPointer<double>(other_type) = static_cast<double>(kCopyValue);
  auto equals_false = ExecuteFunction(*this, "_Int_Equals_<C>_Object", other_type, copy_dst);
  ASSERT_TRUE(equals_false.has_value());
  ExpectStackTopEquals<bool>(*this, false);

  void* less_left = AllocateObjectByName(*this, "Int");
  void* less_right = AllocateObjectByName(*this, "Int");
  ASSERT_NE(less_left, nullptr);
  ASSERT_NE(less_right, nullptr);
  *ovum::vm::runtime::GetDataPointer<int64_t>(less_left) = kLessLeft;
  *ovum::vm::runtime::GetDataPointer<int64_t>(less_right) = kLessRight;
  auto less = ExecuteFunction(*this, "_Int_IsLess_<C>_Object", less_left, less_right);
  ASSERT_TRUE(less.has_value());
  ExpectStackTopEquals<bool>(*this, true);

  auto to_string = ExecuteFunction(*this, "_Int_ToString_<C>", int_obj);
  ASSERT_TRUE(to_string.has_value());
  ASSERT_FALSE(memory_.machine_stack.empty());
  auto string_obj = std::get<void*>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  auto* string_data = ovum::vm::runtime::GetDataPointer<std::string>(string_obj);
  EXPECT_EQ(*string_data, std::to_string(kValue));

  auto hash = ExecuteFunction(*this, "_Int_GetHash_<C>", int_obj);
  ASSERT_TRUE(hash.has_value());
  ExpectStackTopEquals<int64_t>(*this, static_cast<int64_t>(std::hash<int64_t>{}(kValue)));
}

TEST_F(BuiltinTestSuite, PrimitiveFloatCharByteBoolMethods) {
  constexpr double kFloatValue = 3.5;
  constexpr double kFloatCopy = -1.25;
  constexpr char kCharValue = 'q';
  constexpr uint8_t kByteValue = 0xAB;
  constexpr bool kBoolFalse = false;
  constexpr bool kBoolTrue = true;

  // Float constructor/copy/equals
  void* float_obj = AllocateObjectByName(*this, "Float");
  ASSERT_NE(float_obj, nullptr);
  ASSERT_TRUE(ExecuteFunction(*this, "_Float_float", float_obj, kFloatValue).has_value());
  ExpectStackTopPointer(*this, float_obj);
  EXPECT_DOUBLE_EQ(*ovum::vm::runtime::GetDataPointer<double>(float_obj), kFloatValue);

  void* float_src = AllocateObjectByName(*this, "Float");
  void* float_dst = AllocateObjectByName(*this, "Float");
  ASSERT_NE(float_src, nullptr);
  ASSERT_NE(float_dst, nullptr);
  *ovum::vm::runtime::GetDataPointer<double>(float_src) = kFloatCopy;
  ASSERT_TRUE(ExecuteFunction(*this, "_Float_Float", float_dst, float_src).has_value());
  ExpectStackTopPointer(*this, float_dst);
  EXPECT_DOUBLE_EQ(*ovum::vm::runtime::GetDataPointer<double>(float_dst), kFloatCopy);
  ASSERT_TRUE(ExecuteFunction(*this, "_Float_Equals_<C>_Object", float_dst, float_src).has_value());
  ExpectStackTopEquals<bool>(*this, true);

  // Char methods
  void* char_obj = AllocateObjectByName(*this, "Char");
  ASSERT_NE(char_obj, nullptr);
  ASSERT_TRUE(ExecuteFunction(*this, "_Char_char", char_obj, kCharValue).has_value());
  ExpectStackTopPointer(*this, char_obj);
  ASSERT_TRUE(ExecuteFunction(*this, "_Char_ToString_<C>", char_obj).has_value());
  auto char_string = std::get<void*>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  auto* char_str_data = ovum::vm::runtime::GetDataPointer<std::string>(char_string);
  EXPECT_EQ(*char_str_data, std::string(1, kCharValue));
  ASSERT_TRUE(ExecuteFunction(*this, "_Char_GetHash_<C>", char_obj).has_value());
  ExpectStackTopEquals<int64_t>(*this, static_cast<int64_t>(std::hash<char>{}(kCharValue)));

  // Byte methods
  void* byte_obj = AllocateObjectByName(*this, "Byte");
  ASSERT_NE(byte_obj, nullptr);
  ASSERT_TRUE(ExecuteFunction(*this, "_Byte_byte", byte_obj, kByteValue).has_value());
  ExpectStackTopPointer(*this, byte_obj);
  ASSERT_TRUE(ExecuteFunction(*this, "_Byte_ToString_<C>", byte_obj).has_value());
  auto byte_str_obj = std::get<void*>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  auto* byte_str = ovum::vm::runtime::GetDataPointer<std::string>(byte_str_obj);
  EXPECT_EQ(*byte_str, std::to_string(kByteValue));
  ASSERT_TRUE(ExecuteFunction(*this, "_Byte_GetHash_<C>", byte_obj).has_value());
  ExpectStackTopEquals<int64_t>(*this, static_cast<int64_t>(std::hash<uint8_t>{}(kByteValue)));

  // Bool methods with custom IsLess
  void* bool_false = AllocateObjectByName(*this, "Bool");
  void* bool_true = AllocateObjectByName(*this, "Bool");
  ASSERT_NE(bool_false, nullptr);
  ASSERT_NE(bool_true, nullptr);
  *ovum::vm::runtime::GetDataPointer<bool>(bool_false) = kBoolFalse;
  *ovum::vm::runtime::GetDataPointer<bool>(bool_true) = kBoolTrue;
  ASSERT_TRUE(ExecuteFunction(*this, "_Bool_Equals_<C>_Object", bool_true, bool_true).has_value());
  ExpectStackTopEquals<bool>(*this, true);
  ASSERT_TRUE(ExecuteFunction(*this, "_Bool_IsLess_<C>_Object", bool_false, bool_true).has_value());
  ExpectStackTopEquals<bool>(*this, true);

  // Destructors
}

TEST_F(BuiltinTestSuite, NullableAndStringMethods) {
  constexpr std::string_view kText = "hello";
  constexpr std::string_view kOtherText = "world";

  void* nullable_obj = AllocateObjectByName(*this, "Nullable");
  ASSERT_NE(nullable_obj, nullptr);
  void* wrapped_str = AllocateObjectByName(*this, "String");
  ASSERT_NE(wrapped_str, nullptr);
  auto* str_ptr = ovum::vm::runtime::GetDataPointer<std::string>(wrapped_str);
  new (str_ptr) std::string(kText);

  ASSERT_TRUE(ExecuteFunction(*this, "_Nullable_Object", nullable_obj, wrapped_str).has_value());
  ExpectStackTopPointer(*this, nullable_obj);
  auto* nullable_val = ovum::vm::runtime::GetDataPointer<void*>(nullable_obj);
  EXPECT_EQ(*nullable_val, wrapped_str);

  // String copy/equals/isLess/hash/length/toUtf8Bytes
  void* string_obj = AllocateObjectByName(*this, "String");
  ASSERT_NE(string_obj, nullptr);
  new (ovum::vm::runtime::GetDataPointer<std::string>(string_obj)) std::string(kText);
  void* string_copy = AllocateObjectByName(*this, "String");
  ASSERT_NE(string_copy, nullptr);
  ASSERT_TRUE(ExecuteFunction(*this, "_String_String", string_copy, string_obj).has_value());
  ExpectStackTopPointer(*this, string_copy);
  EXPECT_EQ(*ovum::vm::runtime::GetDataPointer<std::string>(string_copy), kText);

  ASSERT_TRUE(ExecuteFunction(*this, "_String_Equals_<C>_Object", string_copy, string_obj).has_value());
  ExpectStackTopEquals<bool>(*this, true);

  void* string_other = AllocateObjectByName(*this, "String");
  ASSERT_NE(string_other, nullptr);
  new (ovum::vm::runtime::GetDataPointer<std::string>(string_other)) std::string(kOtherText);
  ASSERT_TRUE(ExecuteFunction(*this, "_String_IsLess_<C>_Object", string_other, string_obj).has_value());
  ExpectStackTopEquals<bool>(*this, kOtherText < kText);

  ASSERT_TRUE(ExecuteFunction(*this, "_String_GetHash_<C>", string_obj).has_value());
  ExpectStackTopEquals<int64_t>(*this, static_cast<int64_t>(std::hash<std::string>{}(std::string(kText))));

  ASSERT_TRUE(ExecuteFunction(*this, "_String_Length_<C>", string_obj).has_value());
  ExpectStackTopEquals<int64_t>(*this, static_cast<int64_t>(kText.size()));

  ASSERT_TRUE(ExecuteFunction(*this, "_String_ToUtf8Bytes_<C>", string_obj).has_value());
  auto utf8_obj = std::get<void*>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  auto* byte_array = ovum::vm::runtime::GetDataPointer<ovum::vm::runtime::ByteArray>(utf8_obj);
  ASSERT_NE(byte_array, nullptr);
  EXPECT_EQ(byte_array->Size(), kText.size() + 1);
  EXPECT_EQ(static_cast<char>(byte_array->Data()[kText.size()]), '\0');
  EXPECT_EQ(std::string(reinterpret_cast<char*>(byte_array->Data())), kText);
}

TEST_F(BuiltinTestSuite, FundamentalArrayMethods) {
  constexpr int64_t kIntDefault = 1;
  constexpr int64_t kIntInsert = 5;
  constexpr int64_t kIntSet = 9;
  constexpr double kFloatDefault = 2.0;
  constexpr double kFloatInsert = -3.5;
  constexpr char kCharDefault = 'a';
  constexpr char kCharInsert = 'z';
  constexpr bool kBoolDefault = true;
  constexpr bool kBoolInsert = false;
  constexpr int64_t kInitialSize = 2;
  constexpr int64_t kReserveSize = 6;

  auto run_int_array = [&]() {
    void* obj = AllocateObjectByName(*this, "IntArray");
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_int_int", obj, kInitialSize, kIntDefault).has_value());
    ExpectStackTopPointer(*this, obj);

    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_Add_<M>_int", obj, kIntInsert).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_Length_<C>", obj).has_value());
    ExpectStackTopEquals<int64_t>(*this, kInitialSize + 1);

    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_InsertAt_<M>_int_int", obj, 0, kIntInsert).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_SetAt_<M>_int_int", obj, -1, kIntSet).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_GetAt_<C>_int", obj, -1).has_value());
    ExpectStackTopEquals<int64_t>(*this, kIntSet);

    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_RemoveAt_<M>_int", obj, -2).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_Reserve_<M>_int", obj, kReserveSize).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_Capacity_<C>", obj).has_value());
    auto cap = std::get<int64_t>(memory_.machine_stack.top());
    memory_.machine_stack.pop();
    EXPECT_GE(cap, kReserveSize);

    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_Clear_<M>", obj).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_Length_<C>", obj).has_value());
    ExpectStackTopEquals<int64_t>(*this, 0);

    ASSERT_TRUE(ExecuteFunction(*this, "_IntArray_ShrinkToFit_<M>", obj).has_value());
  };

  auto run_float_array = [&]() {
    void* obj = AllocateObjectByName(*this, "FloatArray");
    ASSERT_TRUE(ExecuteFunction(*this, "_FloatArray_int_float", obj, kInitialSize, kFloatDefault).has_value());
    ExpectStackTopPointer(*this, obj);
    ASSERT_TRUE(ExecuteFunction(*this, "_FloatArray_Add_<M>_float", obj, kFloatInsert).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_FloatArray_GetAt_<C>_int", obj, 0).has_value());
    ExpectStackTopEquals<double>(*this, kFloatDefault);
  };

  auto run_char_array = [&]() {
    void* obj = AllocateObjectByName(*this, "CharArray");
    ASSERT_TRUE(ExecuteFunction(*this, "_CharArray_int_char", obj, kInitialSize, kCharDefault).has_value());
    ExpectStackTopPointer(*this, obj);
    ASSERT_TRUE(ExecuteFunction(*this, "_CharArray_InsertAt_<M>_int_char", obj, 1, kCharInsert).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_CharArray_GetAt_<C>_int", obj, 1).has_value());
    ExpectStackTopEquals<char>(*this, kCharInsert);
  };

  auto run_bool_array = [&]() {
    void* obj = AllocateObjectByName(*this, "BoolArray");
    ASSERT_TRUE(ExecuteFunction(*this, "_BoolArray_int_bool", obj, kInitialSize, kBoolDefault).has_value());
    ExpectStackTopPointer(*this, obj);
    ASSERT_TRUE(ExecuteFunction(*this, "_BoolArray_SetAt_<M>_int_bool", obj, 0, kBoolInsert).has_value());
    ASSERT_TRUE(ExecuteFunction(*this, "_BoolArray_GetAt_<C>_int", obj, 0).has_value());
    ExpectStackTopEquals<bool>(*this, kBoolInsert);
  };

  run_int_array();
  run_float_array();
  run_char_array();
  run_bool_array();
}

TEST_F(BuiltinTestSuite, ObjectStringPointerArrayMethods) {
  constexpr int64_t kSize = 1;
  constexpr int64_t kInsertIndex = -1;

  auto make_string_obj = [&](std::string_view text) {
    void* obj = AllocateObjectByName(*this, "String");
    new (ovum::vm::runtime::GetDataPointer<std::string>(obj)) std::string(text);
    return obj;
  };

  void* object_array = AllocateObjectByName(*this, "ObjectArray");
  ASSERT_TRUE(ExecuteFunction(*this, "_ObjectArray_int_Object", object_array, kSize, nullptr).has_value());
  ExpectStackTopPointer(*this, object_array);
  void* stored = make_string_obj("obj");
  ASSERT_TRUE(ExecuteFunction(*this, "_ObjectArray_Add_<M>_Object", object_array, stored).has_value());
  ASSERT_TRUE(ExecuteFunction(*this, "_ObjectArray_GetAt_<C>_int", object_array, kInsertIndex).has_value());
  ExpectStackTopPointer(*this, stored);

  void* string_array = AllocateObjectByName(*this, "StringArray");
  ASSERT_TRUE(ExecuteFunction(*this, "_StringArray_int_String", string_array, kSize, stored).has_value());
  ExpectStackTopPointer(*this, string_array);
  void* another_str = make_string_obj("second");
  ASSERT_TRUE(ExecuteFunction(*this, "_StringArray_InsertAt_<M>_int_String", string_array, 1, another_str).has_value());
  ASSERT_TRUE(ExecuteFunction(*this, "_StringArray_Length_<C>", string_array).has_value());
  ExpectStackTopEquals<int64_t>(*this, kSize + 1);

  void* pointer_array = AllocateObjectByName(*this, "PointerArray");
  ASSERT_TRUE(ExecuteFunction(*this, "_PointerArray_int_Pointer", pointer_array, kSize, stored).has_value());
  ExpectStackTopPointer(*this, pointer_array);
  ASSERT_TRUE(ExecuteFunction(*this, "_PointerArray_SetAt_<M>_int_Pointer", pointer_array, kInsertIndex, another_str)
                  .has_value());
  ASSERT_TRUE(ExecuteFunction(*this, "_PointerArray_GetAt_<C>_int", pointer_array, kInsertIndex).has_value());
  ExpectStackTopPointer(*this, another_str);
}

TEST_F(BuiltinTestSuite, ByteArrayOperations) {
  constexpr int64_t kSize = 3;
  constexpr uint8_t kDefaultByte = 0x01;
  constexpr uint8_t kInsertByte = 0xFF;
  constexpr uint8_t kSetByte = 0x0A;
  constexpr int64_t kAppendIndex = 3;
  constexpr int64_t kWrappedIndex = -1;

  void* byte_array_obj = AllocateObjectByName(*this, "ByteArray");
  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_int_byte", byte_array_obj, kSize, kDefaultByte).has_value());
  ExpectStackTopPointer(*this, byte_array_obj);

  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_Add_<M>_byte", byte_array_obj, kInsertByte).has_value());
  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_InsertAt_<M>_int_byte", byte_array_obj, kAppendIndex, kInsertByte)
                  .has_value());

  ASSERT_TRUE(
      ExecuteFunction(*this, "_ByteArray_SetAt_<M>_int_byte", byte_array_obj, kWrappedIndex, kSetByte).has_value());
  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_GetAt_<C>_int", byte_array_obj, kWrappedIndex).has_value());
  ExpectStackTopEquals<uint8_t>(*this, kSetByte);

  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_RemoveAt_<M>_int", byte_array_obj, kWrappedIndex).has_value());
  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_Length_<C>", byte_array_obj).has_value());
  auto length = std::get<int64_t>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  EXPECT_GE(length, 1);

  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_GetHash_<C>", byte_array_obj).has_value());
  memory_.machine_stack.pop(); // hash value not asserted for determinism here

  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_Clear_<M>", byte_array_obj).has_value());
}

TEST_F(BuiltinTestSuite, ByteArrayFromObjectCreatesView) {
  constexpr int64_t kIntValue = 77;

  void* int_obj = AllocateObjectByName(*this, "Int");
  ASSERT_NE(int_obj, nullptr);
  *ovum::vm::runtime::GetDataPointer<int64_t>(int_obj) = kIntValue;

  void* byte_array_obj = AllocateObjectByName(*this, "ByteArray");
  ASSERT_TRUE(ExecuteFunction(*this, "_ByteArray_Object", byte_array_obj, int_obj).has_value());
  ExpectStackTopPointer(*this, byte_array_obj);

  auto* byte_array = ovum::vm::runtime::GetDataPointer<ovum::vm::runtime::ByteArray>(byte_array_obj);
  ASSERT_NE(byte_array, nullptr);
  EXPECT_GE(byte_array->Size(), sizeof(ovum::vm::runtime::ObjectDescriptor));
  auto* descriptor = reinterpret_cast<ovum::vm::runtime::ObjectDescriptor*>(byte_array->Data());
  EXPECT_EQ(descriptor->vtable_index, vtable_repo_.GetIndexByName("Int").value());

  *ovum::vm::runtime::GetDataPointer<int64_t>(int_obj) = kIntValue + 1;
  auto* int_bytes = reinterpret_cast<int64_t*>(byte_array->Data() + sizeof(ovum::vm::runtime::ObjectDescriptor));
  EXPECT_EQ(*int_bytes, kIntValue + 1);
}

TEST_F(BuiltinTestSuite, PointerMethods) {
  constexpr int64_t kValue = 5;
  constexpr int64_t kOther = 6;

  void* pointee_one = AllocateObjectByName(*this, "Int");
  void* pointee_two = AllocateObjectByName(*this, "Int");
  ASSERT_NE(pointee_one, nullptr);
  ASSERT_NE(pointee_two, nullptr);
  *ovum::vm::runtime::GetDataPointer<int64_t>(pointee_one) = kValue;
  *ovum::vm::runtime::GetDataPointer<int64_t>(pointee_two) = kOther;

  void* pointer_obj = AllocateObjectByName(*this, "Pointer");
  ASSERT_TRUE(ExecuteFunction(*this, "_Pointer_pointer", pointer_obj, pointee_one).has_value());
  ExpectStackTopPointer(*this, pointer_obj);
  ASSERT_TRUE(ExecuteFunction(*this, "_Pointer_Equals_<C>_Object", pointer_obj, pointer_obj).has_value());
  ExpectStackTopEquals<bool>(*this, true);

  void* pointer_copy = AllocateObjectByName(*this, "Pointer");
  ASSERT_TRUE(ExecuteFunction(*this, "_Pointer_Pointer", pointer_copy, pointer_obj).has_value());
  ExpectStackTopPointer(*this, pointer_copy);

  ASSERT_TRUE(ExecuteFunction(*this, "_Pointer_IsLess_<C>_Object", pointer_obj, pointer_copy).has_value());
  memory_.machine_stack.pop(); // ignore specific ordering

  ASSERT_TRUE(ExecuteFunction(*this, "_Pointer_GetHash_<C>", pointer_obj).has_value());
  memory_.machine_stack.pop();
}

TEST_F(BuiltinTestSuite, FileMethods) {
  constexpr std::string_view kModeWrite = "w";
  constexpr std::string_view kModeRead = "r";
  constexpr std::string_view kContent = "file-content";
  constexpr int64_t kReadSize = 4;

  namespace fs = std::filesystem;
  const fs::path temp_path = fs::temp_directory_path() / "ovum_builtin_file_test.txt";
  if (fs::exists(temp_path)) {
    fs::remove(temp_path);
  }

  void* file_obj = AllocateObjectByName(*this, "File");
  ASSERT_TRUE(ExecuteFunction(*this, "_File", file_obj).has_value());
  ExpectStackTopPointer(*this, file_obj);

  void* path_obj = AllocateObjectByName(*this, "String");
  new (ovum::vm::runtime::GetDataPointer<std::string>(path_obj)) std::string(temp_path.string());
  void* write_mode_obj = AllocateObjectByName(*this, "String");
  new (ovum::vm::runtime::GetDataPointer<std::string>(write_mode_obj)) std::string(kModeWrite);
  ASSERT_TRUE(ExecuteFunction(*this, "_File_Open_<M>_String_String", file_obj, path_obj, write_mode_obj).has_value());

  std::vector<uint8_t> bytes(std::begin(kContent), std::end(kContent) - 1);
  void* byte_array_obj = AllocateObjectByName(*this, "ByteArray");
  auto* byte_array = ovum::vm::runtime::GetDataPointer<ovum::vm::runtime::ByteArray>(byte_array_obj);
  new (byte_array) ovum::vm::runtime::ByteArray(bytes.size());
  std::ranges::copy(bytes, byte_array->Data());

  ASSERT_TRUE(ExecuteFunction(*this, "_File_Write_<M>_ByteArray", file_obj, byte_array_obj).has_value());
  ExpectStackTopEquals<int64_t>(*this, static_cast<int64_t>(bytes.size()));
  EXPECT_TRUE(ExecuteFunction(*this, "_File_Close_<M>", file_obj).has_value());

  void* read_mode_obj = AllocateObjectByName(*this, "String");
  new (ovum::vm::runtime::GetDataPointer<std::string>(read_mode_obj)) std::string(kModeRead);
  ASSERT_TRUE(ExecuteFunction(*this, "_File_Open_<M>_String_String", file_obj, path_obj, read_mode_obj).has_value());

  ASSERT_TRUE(ExecuteFunction(*this, "_File_Read_<M>_Int", file_obj, kReadSize).has_value());
  auto read_bytes_obj = std::get<void*>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  auto* read_bytes = ovum::vm::runtime::GetDataPointer<ovum::vm::runtime::ByteArray>(read_bytes_obj);
  ASSERT_NE(read_bytes, nullptr);
  EXPECT_EQ(read_bytes->Size(), static_cast<size_t>(kReadSize));

  ASSERT_TRUE(ExecuteFunction(*this, "_File_ReadLine_<M>", file_obj).has_value());
  auto read_line_obj = std::get<void*>(memory_.machine_stack.top());
  memory_.machine_stack.pop();
  auto* read_line_str = ovum::vm::runtime::GetDataPointer<std::string>(read_line_obj);
  EXPECT_FALSE(read_line_str->empty());
  EXPECT_EQ(read_line_str->front(), '-');

  ASSERT_TRUE(ExecuteFunction(*this, "_File_Eof_<C>", file_obj).has_value());
  ExpectStackTopEquals<bool>(*this, true);

  EXPECT_TRUE(ExecuteFunction(*this, "_File_Close_<M>", file_obj).has_value());
}
