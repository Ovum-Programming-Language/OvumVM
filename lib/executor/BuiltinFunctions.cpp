#include "BuiltinFunctions.hpp"

#include <cstdint>
#include <cstring>
#include <expected>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/Variable.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::vm::execution_tree {

// Template helper for fundamental type constructors (object already allocated, just initialize)
// Stack order: value is on top, object (this) is below
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("Constructor: insufficient arguments (need object and value)"));
  }
  T value = std::get<T>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  T* data_ptr = runtime::GetDataPointer<T>(obj_ptr);
  *data_ptr = value;
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

// Template helper for fundamental type copy constructors
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeCopyConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("CopyConstructor: insufficient arguments (need object and source)"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const T* source_data = runtime::GetDataPointer<const T>(source_obj);
  T* data_ptr = runtime::GetDataPointer<T>(obj_ptr);
  *data_ptr = *source_data;
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

// Template helper for fundamental type destructors (trivial, no cleanup needed)
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeDestructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("Destructor: stack empty"));
  }
  data.memory.machine_stack.pop();
  return ExecutionResult::kNormal;
}

// Template helper for fundamental type Equals
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeEquals(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("Equals: insufficient arguments"));
  }
  void* obj2_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj1_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const T* value1 = runtime::GetDataPointer<const T>(obj1_ptr);
  const T* value2 = runtime::GetDataPointer<const T>(obj2_ptr);
  bool equals = (*value1 == *value2);
  data.memory.machine_stack.push(equals);
  return ExecutionResult::kNormal;
}

// Template helper for fundamental type IsLess
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeIsLess(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("IsLess: insufficient arguments"));
  }
  void* obj2_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj1_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const T* value1 = runtime::GetDataPointer<const T>(obj1_ptr);
  const T* value2 = runtime::GetDataPointer<const T>(obj2_ptr);
  bool is_less = (*value1 < *value2);
  data.memory.machine_stack.push(is_less);
  return ExecutionResult::kNormal;
}

// Template helper for fundamental type ToString
// ToStringFunc is a callable type that takes const T& and returns std::string
template<typename T, typename ToStringFunc>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeToString(PassedExecutionData& data,
                                                                           ToStringFunc to_string_func) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("ToString: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  const T* value = runtime::GetDataPointer<const T>(obj_ptr);

  // Convert to string using the functor (avoid copying the value)
  std::string str = to_string_func(*value);

  // Create String object
  auto vtable_result = data.virtual_table_repository.GetByName("String");
  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("ToString: String vtable not found"));
  }
  const runtime::VirtualTable* string_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("String");
  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto string_obj_result = runtime::AllocateObject(
      *string_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
  if (!string_obj_result.has_value()) {
    return std::unexpected(string_obj_result.error());
  }
  void* string_obj = string_obj_result.value();
  std::string* string_data = runtime::GetDataPointer<std::string>(string_obj);
  new (string_data) std::string(std::move(str)); // Move to avoid copying
  data.memory.machine_stack.push(string_obj);
  return ExecutionResult::kNormal;
}

// Template helper for fundamental type GetHash
// GetHashFunc is a callable type that takes const T& and returns int64_t
template<typename T, typename GetHashFunc>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeGetHash(PassedExecutionData& data,
                                                                          GetHashFunc get_hash_func) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("GetHash: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  const T* value = runtime::GetDataPointer<const T>(obj_ptr);

  // Get hash using the functor (avoid copying the value)
  int64_t hash = get_hash_func(*value);
  data.memory.machine_stack.push(hash);
  return ExecutionResult::kNormal;
}

// Template helper for array constructors (object already allocated)
// Stack order: default_value is on top, then size, then object (this) is at bottom
// For fundamental types, default_value comes as the fundamental type
// For ObjectArray/StringArray/PointerArray, default_value comes as void*
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 3) {
    return std::unexpected(std::runtime_error("ArrayConstructor: insufficient arguments (need object, size, default)"));
  }
  T default_value = std::get<T>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  int64_t size = std::get<int64_t>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::vector<T>* vec_data = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  new (vec_data) std::vector<T>(static_cast<size_t>(size), default_value);
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

// Specialization for ObjectArray/StringArray/PointerArray (default_value is void*)
template<>
inline std::expected<ExecutionResult, std::runtime_error> ArrayConstructor<void*>(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 3) {
    return std::unexpected(std::runtime_error("ArrayConstructor: insufficient arguments (need object, size, default)"));
  }
  void* default_value = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  int64_t size = std::get<int64_t>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::vector<void*>* vec_data = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);
  new (vec_data) std::vector<void*>(static_cast<size_t>(size), default_value);
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

// Template helper for array copy constructors
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayCopyConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ArrayCopyConstructor: insufficient arguments"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<T>* source_vec = runtime::GetDataPointer<const std::vector<T>>(source_obj);
  std::vector<T>* vec_data = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  new (vec_data) std::vector<T>(*source_vec);
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

// Template helper for array destructors
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayDestructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("ArrayDestructor: stack empty"));
  }
  using vector_type = std::vector<T>;
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::vector<T>* vec_data = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  vec_data->~vector_type();
  return ExecutionResult::kNormal;
}

// Template helper for array Equals
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayEquals(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ArrayEquals: insufficient arguments"));
  }
  void* obj2_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj1_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<T>* vec1 = runtime::GetDataPointer<const std::vector<T>>(obj1_ptr);
  const std::vector<T>* vec2 = runtime::GetDataPointer<const std::vector<T>>(obj2_ptr);
  bool equals = (*vec1 == *vec2);
  data.memory.machine_stack.push(equals);
  return ExecutionResult::kNormal;
}

// Template helper for array IsLess
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayIsLess(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ArrayIsLess: insufficient arguments"));
  }
  void* obj2_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj1_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<T>* vec1 = runtime::GetDataPointer<const std::vector<T>>(obj1_ptr);
  const std::vector<T>* vec2 = runtime::GetDataPointer<const std::vector<T>>(obj2_ptr);
  bool is_less = (*vec1 < *vec2);
  data.memory.machine_stack.push(is_less);
  return ExecutionResult::kNormal;
}

// Template helper for array Length
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayLength(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("ArrayLength: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  const std::vector<T>* vec = runtime::GetDataPointer<const std::vector<T>>(obj_ptr);
  int64_t length = static_cast<int64_t>(vec->size());
  data.memory.machine_stack.push(length);
  return ExecutionResult::kNormal;
}

// Template helper for array GetHash
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayGetHash(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("ArrayGetHash: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  const std::vector<T>* vec = runtime::GetDataPointer<const std::vector<T>>(obj_ptr);
  int64_t hash = runtime::HashVector(*vec);
  data.memory.machine_stack.push(hash);
  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree

namespace ovum::vm::runtime {} // namespace ovum::vm::runtime

namespace ovum::vm::execution_tree {

// Int methods - using templates
std::expected<ExecutionResult, std::runtime_error> IntConstructor(PassedExecutionData& data) {
  return FundamentalTypeConstructor<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntCopyConstructor(PassedExecutionData& data) {
  return FundamentalTypeCopyConstructor<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntIsLess(PassedExecutionData& data) {
  return FundamentalTypeIsLess<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntToString(PassedExecutionData& data) {
  return FundamentalTypeToString<int64_t>(data, [](const int64_t& val) { return std::to_string(val); });
}

std::expected<ExecutionResult, std::runtime_error> IntGetHash(PassedExecutionData& data) {
  return FundamentalTypeGetHash<int64_t>(
      data, [](const int64_t& val) { return static_cast<int64_t>(std::hash<int64_t>{}(val)); });
}

std::expected<ExecutionResult, std::runtime_error> FloatToString(PassedExecutionData& data) {
  return FundamentalTypeToString<double>(data, [](const double& val) { return std::to_string(val); });
}

std::expected<ExecutionResult, std::runtime_error> FloatGetHash(PassedExecutionData& data) {
  return FundamentalTypeGetHash<double>(
      data, [](const double& val) { return static_cast<int64_t>(std::hash<double>{}(val)); });
}

// Float methods - using templates
std::expected<ExecutionResult, std::runtime_error> FloatConstructor(PassedExecutionData& data) {
  return FundamentalTypeConstructor<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatCopyConstructor(PassedExecutionData& data) {
  return FundamentalTypeCopyConstructor<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatIsLess(PassedExecutionData& data) {
  return FundamentalTypeIsLess<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharToString(PassedExecutionData& data) {
  return FundamentalTypeToString<char>(data, [](const char& val) { return std::string(1, val); });
}

std::expected<ExecutionResult, std::runtime_error> CharGetHash(PassedExecutionData& data) {
  return FundamentalTypeGetHash<char>(data,
                                      [](const char& val) { return static_cast<int64_t>(std::hash<char>{}(val)); });
}

// Char methods - using templates
std::expected<ExecutionResult, std::runtime_error> CharConstructor(PassedExecutionData& data) {
  return FundamentalTypeConstructor<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharCopyConstructor(PassedExecutionData& data) {
  return FundamentalTypeCopyConstructor<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharIsLess(PassedExecutionData& data) {
  return FundamentalTypeIsLess<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteToString(PassedExecutionData& data) {
  return FundamentalTypeToString<uint8_t>(data,
                                          [](const uint8_t& val) { return std::to_string(static_cast<int>(val)); });
}

std::expected<ExecutionResult, std::runtime_error> ByteGetHash(PassedExecutionData& data) {
  return FundamentalTypeGetHash<uint8_t>(
      data, [](const uint8_t& val) { return static_cast<int64_t>(std::hash<uint8_t>{}(val)); });
}

// Byte methods - using templates
std::expected<ExecutionResult, std::runtime_error> ByteConstructor(PassedExecutionData& data) {
  return FundamentalTypeConstructor<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteCopyConstructor(PassedExecutionData& data) {
  return FundamentalTypeCopyConstructor<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteIsLess(PassedExecutionData& data) {
  return FundamentalTypeIsLess<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolToString(PassedExecutionData& data) {
  return FundamentalTypeToString<bool>(data, [](const bool& val) { return val ? "true" : "false"; });
}

std::expected<ExecutionResult, std::runtime_error> BoolGetHash(PassedExecutionData& data) {
  return FundamentalTypeGetHash<bool>(data,
                                      [](const bool& val) { return static_cast<int64_t>(std::hash<bool>{}(val)); });
}

// Bool methods - using templates
std::expected<ExecutionResult, std::runtime_error> BoolConstructor(PassedExecutionData& data) {
  return FundamentalTypeConstructor<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolCopyConstructor(PassedExecutionData& data) {
  return FundamentalTypeCopyConstructor<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolIsLess(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("Bool::IsLess: insufficient arguments"));
  }
  void* obj2_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj1_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const bool* value1 = runtime::GetDataPointer<const bool>(obj1_ptr);
  const bool* value2 = runtime::GetDataPointer<const bool>(obj2_ptr);
  bool is_less = (!*value1 && *value2); // false < true
  data.memory.machine_stack.push(is_less);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NullableDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> StringToString(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("String::ToString: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  // Return self
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringGetHash(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("String::GetHash: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::string* str = runtime::GetDataPointer<std::string>(obj_ptr);
  int64_t hash = static_cast<int64_t>(std::hash<std::string>{}(*str));
  data.memory.machine_stack.push(hash);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringLength(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("String::Length: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::string* str = runtime::GetDataPointer<std::string>(obj_ptr);
  int64_t length = static_cast<int64_t>(str->length());
  data.memory.machine_stack.push(length);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringToUtf8Bytes(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("String::ToUtf8Bytes: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::string* str = runtime::GetDataPointer<std::string>(obj_ptr);

  // Create ByteArray object
  auto vtable_result = data.virtual_table_repository.GetByName("ByteArray");
  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("String::ToUtf8Bytes: ByteArray vtable not found"));
  }
  const runtime::VirtualTable* byte_array_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("ByteArray");
  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto byte_array_obj_result = AllocateObject(
      *byte_array_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
  if (!byte_array_obj_result.has_value()) {
    return std::unexpected(byte_array_obj_result.error());
  }
  void* byte_array_obj = byte_array_obj_result.value();
  std::vector<uint8_t>* vec_data = runtime::GetDataPointer<std::vector<uint8_t>>(byte_array_obj);
  new (vec_data) std::vector<uint8_t>(str->begin(), str->end());
  data.memory.machine_stack.push(byte_array_obj);
  return ExecutionResult::kNormal;
}

// String methods
std::expected<ExecutionResult, std::runtime_error> StringConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("String::Constructor: insufficient arguments"));
  }
  void* source_string_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::string* source_string = runtime::GetDataPointer<const std::string>(source_string_obj);
  std::string* string_data = runtime::GetDataPointer<std::string>(obj_ptr);
  new (string_data) std::string(*source_string);
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringCopyConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("String::CopyConstructor: insufficient arguments"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::string* source_string = runtime::GetDataPointer<const std::string>(source_obj);
  std::string* string_data = runtime::GetDataPointer<std::string>(obj_ptr);
  new (string_data) std::string(*source_string);
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringDestructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("String::Destructor: stack empty"));
  }
  using string_type = std::string;
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::string* string_data = runtime::GetDataPointer<std::string>(obj_ptr);
  string_data->~string_type();
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<std::string>(data);
}

std::expected<ExecutionResult, std::runtime_error> StringIsLess(PassedExecutionData& data) {
  return FundamentalTypeIsLess<std::string>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayLength(PassedExecutionData& data) {
  return ArrayLength<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayLength(PassedExecutionData& data) {
  return ArrayLength<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayLength(PassedExecutionData& data) {
  return ArrayLength<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayLength(PassedExecutionData& data) {
  return ArrayLength<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayLength(PassedExecutionData& data) {
  return ArrayLength<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayLength(PassedExecutionData& data) {
  return ArrayLength<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayLength(PassedExecutionData& data) {
  return ObjectArrayLength(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayGetHash(PassedExecutionData& data) {
  return ObjectArrayGetHash(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerGetHash(PassedExecutionData& data) {
  return FundamentalTypeGetHash<void*>(data, [](const void* val) {
    return static_cast<int64_t>(std::hash<int64_t>{}(static_cast<int64_t>(reinterpret_cast<uintptr_t>(val))));
  });
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayLength(PassedExecutionData& data) {
  return ObjectArrayLength(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayGetHash(PassedExecutionData& data) {
  return ObjectArrayGetHash(data);
}

// File methods
std::expected<ExecutionResult, std::runtime_error> FileOpen(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 3) {
    return std::unexpected(std::runtime_error("File::Open: insufficient arguments (need file, path and mode)"));
  }
  // Pop mode (String), then path (String), then file (File)
  void* mode_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* path_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* file_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::string* path = runtime::GetDataPointer<std::string>(path_obj);
  std::string* mode = runtime::GetDataPointer<std::string>(mode_obj);
  std::fstream* file = runtime::GetDataPointer<std::fstream>(file_obj);

  // Parse mode string
  std::ios_base::openmode open_mode = std::ios_base::in | std::ios_base::out;
  if (*mode == "r") {
    open_mode = std::ios_base::in;
  } else if (*mode == "w") {
    open_mode = std::ios_base::out | std::ios_base::trunc;
  } else if (*mode == "a") {
    open_mode = std::ios_base::out | std::ios_base::app;
  } else if (*mode == "r+") {
    open_mode = std::ios_base::in | std::ios_base::out;
  } else if (*mode == "w+") {
    open_mode = std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
  } else if (*mode == "a+") {
    open_mode = std::ios_base::in | std::ios_base::out | std::ios_base::app;
  }

  // Close if already open
  if (file->is_open()) {
    file->close();
  }

  // Open file
  file->open(*path, open_mode);
  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Open: failed to open file: " + *path));
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileClose(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::Close: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (file->is_open()) {
    file->close();
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileIsOpen(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::IsOpen: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  bool is_open = file->is_open();
  data.memory.machine_stack.push(is_open);
  return ExecutionResult::kNormal;
}

// Array methods - using templates
// IntArray methods
std::expected<ExecutionResult, std::runtime_error> IntArrayConstructor(PassedExecutionData& data) {
  return ArrayConstructor<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayCopyConstructor(PassedExecutionData& data) {
  return ArrayCopyConstructor<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayDestructor(PassedExecutionData& data) {
  return ArrayDestructor<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayEquals(PassedExecutionData& data) {
  return ArrayEquals<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayIsLess(PassedExecutionData& data) {
  return ArrayIsLess<int64_t>(data);
}

// FloatArray methods
std::expected<ExecutionResult, std::runtime_error> FloatArrayConstructor(PassedExecutionData& data) {
  return ArrayConstructor<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayCopyConstructor(PassedExecutionData& data) {
  return ArrayCopyConstructor<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayDestructor(PassedExecutionData& data) {
  return ArrayDestructor<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayEquals(PassedExecutionData& data) {
  return ArrayEquals<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayIsLess(PassedExecutionData& data) {
  return ArrayIsLess<double>(data);
}

// CharArray methods
std::expected<ExecutionResult, std::runtime_error> CharArrayConstructor(PassedExecutionData& data) {
  return ArrayConstructor<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayCopyConstructor(PassedExecutionData& data) {
  return ArrayCopyConstructor<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayDestructor(PassedExecutionData& data) {
  return ArrayDestructor<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayEquals(PassedExecutionData& data) {
  return ArrayEquals<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayIsLess(PassedExecutionData& data) {
  return ArrayIsLess<char>(data);
}

// ByteArray methods
std::expected<ExecutionResult, std::runtime_error> ByteArrayConstructor(PassedExecutionData& data) {
  return ArrayConstructor<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayCopyConstructor(PassedExecutionData& data) {
  return ArrayCopyConstructor<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayDestructor(PassedExecutionData& data) {
  return ArrayDestructor<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayEquals(PassedExecutionData& data) {
  return ArrayEquals<uint8_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayIsLess(PassedExecutionData& data) {
  return ArrayIsLess<uint8_t>(data);
}

// ByteArray view casting constructors (from other array types)
// These create ByteArray by interpreting the raw bytes of other array types
std::expected<ExecutionResult, std::runtime_error> ByteArrayFromIntArray(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ByteArray::FromIntArray: insufficient arguments"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<int64_t>* source_vec = runtime::GetDataPointer<const std::vector<int64_t>>(source_obj);
  std::vector<uint8_t>* vec_data = runtime::GetDataPointer<std::vector<uint8_t>>(obj_ptr);

  // Convert int64_t array to byte array by interpreting raw bytes
  size_t byte_count = source_vec->size() * sizeof(int64_t);
  new (vec_data) std::vector<uint8_t>(byte_count);
  std::memcpy(vec_data->data(), source_vec->data(), byte_count);

  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayFromFloatArray(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ByteArray::FromFloatArray: insufficient arguments"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<double>* source_vec = runtime::GetDataPointer<const std::vector<double>>(source_obj);
  std::vector<uint8_t>* vec_data = runtime::GetDataPointer<std::vector<uint8_t>>(obj_ptr);

  // Convert double array to byte array by interpreting raw bytes
  size_t byte_count = source_vec->size() * sizeof(double);
  new (vec_data) std::vector<uint8_t>(byte_count);
  std::memcpy(vec_data->data(), source_vec->data(), byte_count);

  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayFromCharArray(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ByteArray::FromCharArray: insufficient arguments"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<char>* source_vec = runtime::GetDataPointer<const std::vector<char>>(source_obj);
  std::vector<uint8_t>* vec_data = runtime::GetDataPointer<std::vector<uint8_t>>(obj_ptr);

  // Convert char array to byte array (direct copy, char and uint8_t are compatible)
  new (vec_data) std::vector<uint8_t>(source_vec->begin(), source_vec->end());

  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayFromBoolArray(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("ByteArray::FromBoolArray: insufficient arguments"));
  }
  void* source_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  const std::vector<bool>* source_vec = runtime::GetDataPointer<const std::vector<bool>>(source_obj);
  std::vector<uint8_t>* vec_data = runtime::GetDataPointer<std::vector<uint8_t>>(obj_ptr);

  // Convert bool array to byte array (bool is stored as bits in vector<bool>, so we convert each to uint8_t)
  new (vec_data) std::vector<uint8_t>();
  vec_data->reserve(source_vec->size());
  for (bool val : *source_vec) {
    vec_data->push_back(val ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0));
  }

  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

// BoolArray methods
std::expected<ExecutionResult, std::runtime_error> BoolArrayConstructor(PassedExecutionData& data) {
  return ArrayConstructor<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayCopyConstructor(PassedExecutionData& data) {
  return ArrayCopyConstructor<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayDestructor(PassedExecutionData& data) {
  return ArrayDestructor<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayEquals(PassedExecutionData& data) {
  return ArrayEquals<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayIsLess(PassedExecutionData& data) {
  return ArrayIsLess<bool>(data);
}

// ObjectArray methods
std::expected<ExecutionResult, std::runtime_error> ObjectArrayConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 3) {
    return std::unexpected(std::runtime_error("ObjectArray::Constructor: insufficient arguments"));
  }
  void* default_value = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  int64_t size = std::get<int64_t>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::vector<void*>* vec_data = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);
  new (vec_data) std::vector<void*>(static_cast<size_t>(size), default_value);
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayCopyConstructor(PassedExecutionData& data) {
  return ArrayCopyConstructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayDestructor(PassedExecutionData& data) {
  return ArrayDestructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayEquals(PassedExecutionData& data) {
  return ArrayEquals<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayIsLess(PassedExecutionData& data) {
  return ArrayIsLess<void*>(data);
}

// StringArray methods (same as ObjectArray, but typed)
std::expected<ExecutionResult, std::runtime_error> StringArrayConstructor(PassedExecutionData& data) {
  return ObjectArrayConstructor(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayCopyConstructor(PassedExecutionData& data) {
  return ObjectArrayCopyConstructor(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayDestructor(PassedExecutionData& data) {
  return ObjectArrayDestructor(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayEquals(PassedExecutionData& data) {
  return ObjectArrayEquals(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayIsLess(PassedExecutionData& data) {
  return ObjectArrayIsLess(data);
}

// PointerArray methods
std::expected<ExecutionResult, std::runtime_error> PointerArrayConstructor(PassedExecutionData& data) {
  return ObjectArrayConstructor(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayCopyConstructor(PassedExecutionData& data) {
  return ObjectArrayCopyConstructor(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayDestructor(PassedExecutionData& data) {
  return ObjectArrayDestructor(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayEquals(PassedExecutionData& data) {
  return ObjectArrayEquals(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayIsLess(PassedExecutionData& data) {
  return ObjectArrayIsLess(data);
}

// Pointer methods - using templates
std::expected<ExecutionResult, std::runtime_error> PointerConstructor(PassedExecutionData& data) {
  return FundamentalTypeConstructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerCopyConstructor(PassedExecutionData& data) {
  return FundamentalTypeCopyConstructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerEquals(PassedExecutionData& data) {
  return FundamentalTypeEquals<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerIsLess(PassedExecutionData& data) {
  return FundamentalTypeIsLess<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> FileRead(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("File::Read: insufficient arguments (need file and size)"));
  }
  int64_t size = std::get<int64_t>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Read: file is not open"));
  }

  // Read bytes
  std::vector<uint8_t> buffer(static_cast<size_t>(size));
  file->read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
  size_t bytes_read = static_cast<size_t>(file->gcount());
  buffer.resize(bytes_read);

  // Create ByteArray object
  auto vtable_result = data.virtual_table_repository.GetByName("ByteArray");
  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("File::Read: ByteArray vtable not found"));
  }
  const runtime::VirtualTable* byte_array_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("ByteArray");
  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto byte_array_obj_result = AllocateObject(
      *byte_array_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
  if (!byte_array_obj_result.has_value()) {
    return std::unexpected(byte_array_obj_result.error());
  }
  void* byte_array_obj = byte_array_obj_result.value();
  std::vector<uint8_t>* vec_data = runtime::GetDataPointer<std::vector<uint8_t>>(byte_array_obj);
  new (vec_data) std::vector<uint8_t>(std::move(buffer));
  data.memory.machine_stack.push(byte_array_obj);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileWrite(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("File::Write: insufficient arguments (need file and data)"));
  }
  void* byte_array_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);
  std::vector<uint8_t>* data_vec = runtime::GetDataPointer<std::vector<uint8_t>>(byte_array_obj);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Write: file is not open"));
  }

  // Write bytes
  file->write(reinterpret_cast<const char*>(data_vec->data()), static_cast<std::streamsize>(data_vec->size()));
  if (file->fail()) {
    return std::unexpected(std::runtime_error("File::Write: write failed"));
  }

  int64_t bytes_written = static_cast<int64_t>(data_vec->size());
  data.memory.machine_stack.push(bytes_written);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileReadLine(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::ReadLine: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::ReadLine: file is not open"));
  }

  // Read line
  std::string line;
  if (!std::getline(*file, line)) {
    // EOF or error - return empty string
    line = "";
  }

  // Create String object
  auto vtable_result = data.virtual_table_repository.GetByName("String");
  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("File::ReadLine: String vtable not found"));
  }
  const runtime::VirtualTable* string_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("String");
  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto string_obj_result =
      AllocateObject(*string_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
  if (!string_obj_result.has_value()) {
    return std::unexpected(string_obj_result.error());
  }
  void* string_obj = string_obj_result.value();
  std::string* string_data = runtime::GetDataPointer<std::string>(string_obj);
  new (string_data) std::string(line);
  data.memory.machine_stack.push(string_obj);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileWriteLine(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("File::WriteLine: insufficient arguments (need file and line)"));
  }
  void* line_obj = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);
  std::string* line = runtime::GetDataPointer<std::string>(line_obj);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::WriteLine: file is not open"));
  }

  // Write line with newline
  *file << *line << '\n';
  if (file->fail()) {
    return std::unexpected(std::runtime_error("File::WriteLine: write failed"));
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileSeek(PassedExecutionData& data) {
  if (data.memory.machine_stack.size() < 2) {
    return std::unexpected(std::runtime_error("File::Seek: insufficient arguments (need file and position)"));
  }
  int64_t position = std::get<int64_t>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Seek: file is not open"));
  }

  file->seekg(static_cast<std::streampos>(position));
  file->seekp(static_cast<std::streampos>(position));
  if (file->fail()) {
    return std::unexpected(std::runtime_error("File::Seek: seek failed"));
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileTell(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::Tell: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Tell: file is not open"));
  }

  std::streampos pos = file->tellg();
  int64_t position = static_cast<int64_t>(pos);
  data.memory.machine_stack.push(position);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileEof(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::Eof: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Eof: file is not open"));
  }

  bool eof = file->eof();
  data.memory.machine_stack.push(eof);
  return ExecutionResult::kNormal;
}

// File methods (constructor, destructor)
std::expected<ExecutionResult, std::runtime_error> FileConstructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::Constructor: stack empty"));
  }
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();

  std::fstream* file_data = runtime::GetDataPointer<std::fstream>(obj_ptr);
  new (file_data) std::fstream();
  data.memory.machine_stack.push(obj_ptr);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileDestructor(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("File::Destructor: stack empty"));
  }
  using fstream_type = std::fstream;
  void* obj_ptr = std::get<void*>(data.memory.machine_stack.top());
  data.memory.machine_stack.pop();
  std::fstream* file_data = runtime::GetDataPointer<std::fstream>(obj_ptr);
  if (file_data->is_open()) {
    file_data->close();
  }
  file_data->~fstream_type();
  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree
