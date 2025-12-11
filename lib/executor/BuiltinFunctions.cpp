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
#include "lib/runtime/ByteArray.hpp"
#include "lib/runtime/Variable.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::vm::execution_tree {

// Helper to check if two objects have the same type (same vtable_index)
inline bool AreSameType(void* obj1_ptr, void* obj2_ptr) {
  const auto* desc1 = reinterpret_cast<const runtime::ObjectDescriptor*>(obj1_ptr);
  const auto* desc2 = reinterpret_cast<const runtime::ObjectDescriptor*>(obj2_ptr);
  return desc1->vtable_index == desc2->vtable_index;
}

// Helper to compute circular index for array operations
// For insert operations (allow_append=true), handles empty arrays and allows index == size for append
// For other operations (allow_append=false), requires non-empty array and wraps index to valid range
inline size_t ComputeCircularIndex(int64_t index, size_t size, bool allow_append) {
  // Empty array: any index becomes 0 (append)
  if (size == 0 && allow_append) {
    return 0;
  }

  // Wrap index: for size 5, index 6→1, 11→1, -1→4
  auto size_i64 = static_cast<int64_t>(size);
  index = ((index % size_i64) + size_i64) % size_i64;

  // Check if index equals size by modulo operation (append case)
  if (index == 0 && allow_append) {
    index = size_i64; // Append
  }

  return static_cast<size_t>(index);
}

// Template helper for fundamental type constructors (object already allocated, just initialize)
// Arguments: object (this) is first, value is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<T>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("Constructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  T value = std::get<T>(data.memory.stack_frames.top().local_variables[1]);
  T* data_ptr = runtime::GetDataPointer<T>(obj_ptr);
  *data_ptr = value;
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

// Template helper for fundamental type copy constructors
// Arguments: object (this) is first, source is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeCopyConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("CopyConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* source_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  const T* source_data = runtime::GetDataPointer<const T>(source_obj);
  T* data_ptr = runtime::GetDataPointer<T>(obj_ptr);
  *data_ptr = *source_data;
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

// Template helper for fundamental type destructors (trivial, no cleanup needed)
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeDestructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("Destructor: invalid argument types"));
  }

  return ExecutionResult::kNormal;
}

// Template helper for fundamental type Equals
// Arguments: obj1 is first, obj2 is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeEquals(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("Equals: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const T* value1 = runtime::GetDataPointer<const T>(obj1_ptr);
  const T* value2 = runtime::GetDataPointer<const T>(obj2_ptr);
  bool equals = (*value1 == *value2);
  data.memory.machine_stack.emplace(equals);

  return ExecutionResult::kNormal;
}

// Template helper for fundamental type IsLess
// Arguments: obj1 is first, obj2 is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeIsLess(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("IsLess: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const T* value1 = runtime::GetDataPointer<const T>(obj1_ptr);
  const T* value2 = runtime::GetDataPointer<const T>(obj2_ptr);
  bool is_less = (*value1 < *value2);
  data.memory.machine_stack.emplace(is_less);

  return ExecutionResult::kNormal;
}

// Template helper for fundamental type ToString
// ToStringFunc is a callable type that takes const T& and returns std::string
// Arguments: object is first
template<typename T, typename ToStringFunc>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeToString(PassedExecutionData& data,
                                                                           ToStringFunc to_string_func) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ToString: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const T* value = runtime::GetDataPointer<const T>(obj_ptr);

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

  auto string_obj_result = runtime::AllocateObject(*string_vtable,
                                                   static_cast<uint32_t>(vtable_index_result.value()),
                                                   data.memory.object_repository,
                                                   data.allocator);

  if (!string_obj_result.has_value()) {
    return std::unexpected(string_obj_result.error());
  }

  void* string_obj = string_obj_result.value();
  auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
  new (string_data) std::string(std::move(to_string_func(*value))); // Move to avoid copying
  data.memory.machine_stack.emplace(string_obj);

  return ExecutionResult::kNormal;
}

// Template helper for fundamental type GetHash
// GetHashFunc is a callable type that takes const T& and returns int64_t
// Arguments: object is first
template<typename T, typename GetHashFunc>
std::expected<ExecutionResult, std::runtime_error> FundamentalTypeGetHash(PassedExecutionData& data,
                                                                          GetHashFunc get_hash_func) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("GetHash: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const T* value = runtime::GetDataPointer<const T>(obj_ptr);

  // Get hash using the functor (avoid copying the value)
  int64_t hash = get_hash_func(*value);
  data.memory.machine_stack.emplace(hash);

  return ExecutionResult::kNormal;
}

// Template helper for array constructors (object already allocated)
// Arguments: object (this) is first, size is second, default_value is third
// For fundamental types, default_value comes as the fundamental type
// For ObjectArray/StringArray/PointerArray, default_value comes as void*
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<T>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ArrayConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t size = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  T default_value = std::get<T>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec_data = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  new (vec_data) std::vector<T>(static_cast<size_t>(size), default_value);
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

// Specialization for ObjectArray/StringArray/PointerArray (default_value is void*)
// Arguments: object (this) is first, size is second, default_value is third
template<>
std::expected<ExecutionResult, std::runtime_error> ArrayConstructor<void*>(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ArrayConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t size = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  void* default_value = std::get<void*>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec_data = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);
  new (vec_data) std::vector<void*>(static_cast<size_t>(size), default_value);
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

// Template helper for array copy constructors
// Arguments: object (this) is first, source is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayCopyConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayCopyConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* source_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  const auto* source_vec = runtime::GetDataPointer<const std::vector<T>>(source_obj);
  auto* vec_data = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  new (vec_data) std::vector<T>(*source_vec);
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

// Template helper for array destructors
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayDestructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ArrayDestructor: invalid argument types"));
  }

  using vector_type = std::vector<T>;
  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* vec_data = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  vec_data->~vector_type();

  return ExecutionResult::kNormal;
}

// Template helper for array Equals
// Arguments: obj1 is first, obj2 is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayEquals(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayEquals: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const auto* vec1 = runtime::GetDataPointer<const std::vector<T>>(obj1_ptr);
  const auto* vec2 = runtime::GetDataPointer<const std::vector<T>>(obj2_ptr);
  bool equals = (*vec1 == *vec2);
  data.memory.machine_stack.emplace(equals);

  return ExecutionResult::kNormal;
}

// Template helper for array IsLess
// Arguments: obj1 is first, obj2 is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayIsLess(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayIsLess: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const auto* vec1 = runtime::GetDataPointer<const std::vector<T>>(obj1_ptr);
  const auto* vec2 = runtime::GetDataPointer<const std::vector<T>>(obj2_ptr);
  bool is_less = (*vec1 < *vec2);
  data.memory.machine_stack.emplace(is_less);

  return ExecutionResult::kNormal;
}

// Template helper for array Length
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayLength(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ArrayLength: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const auto* vec = runtime::GetDataPointer<const std::vector<T>>(obj_ptr);
  auto length = static_cast<int64_t>(vec->size());
  data.memory.machine_stack.emplace(length);

  return ExecutionResult::kNormal;
}

// Template helper for array GetHash
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayGetHash(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ArrayGetHash: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const auto* vec = runtime::GetDataPointer<const std::vector<T>>(obj_ptr);
  int64_t hash = runtime::HashVector(*vec);
  data.memory.machine_stack.emplace(hash);

  return ExecutionResult::kNormal;
}

// Template helper for array Clear
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayClear(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ArrayClear: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  vec->clear();

  return ExecutionResult::kNormal;
}

// Template helper for array ShrinkToFit
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayShrinkToFit(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ArrayShrinkToFit: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  vec->shrink_to_fit();

  return ExecutionResult::kNormal;
}

// Template helper for array Reserve
// Arguments: object is first, capacity is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayReserve(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayReserve: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t capacity = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  vec->reserve(static_cast<size_t>(capacity));

  return ExecutionResult::kNormal;
}

// Template helper for array Capacity
// Arguments: object is first
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayCapacity(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ArrayCapacity: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const auto* vec = runtime::GetDataPointer<const std::vector<T>>(obj_ptr);
  auto capacity = static_cast<int64_t>(vec->capacity());
  data.memory.machine_stack.emplace(capacity);

  return ExecutionResult::kNormal;
}

// Template helper for array Add
// Arguments: object is first, value is second
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayAdd(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<T>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayAdd: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  T value = std::get<T>(data.memory.stack_frames.top().local_variables[1]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);
  vec->push_back(value);

  return ExecutionResult::kNormal;
}

// Specialization for ObjectArray/StringArray/PointerArray (value is void*)
template<>
std::expected<ExecutionResult, std::runtime_error> ArrayAdd<void*>(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayAdd: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* value = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  auto* vec = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);
  vec->push_back(value);

  return ExecutionResult::kNormal;
}

// Template helper for array RemoveAt
// Arguments: object is first, index is second
// Uses circular indexing: index wraps around array size
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayRemoveAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayRemoveAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);

  if (vec->empty()) {
    return std::unexpected(std::runtime_error("ArrayRemoveAt: cannot remove from empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = vec->size();
  size_t circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                           static_cast<int64_t>(size));

  vec->erase(vec->begin() + static_cast<ptrdiff_t>(circular_index));

  return ExecutionResult::kNormal;
}

// Template helper for array InsertAt
// Arguments: object is first, index is second, value is third
// Uses circular indexing: index wraps around array size
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayInsertAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<T>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ArrayInsertAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  T value = std::get<T>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);

  // Circular indexing: wrap index to valid range
  // For insert, we allow index == size (append), so we handle it specially
  size_t size = vec->size();
  size_t circular_index = 0;

  if (size == 0) {
    // Empty array: any index becomes 0 (append)
    circular_index = 0;
  } else {
    // Check if index equals size (append case)
    if (index == static_cast<int64_t>(size)) {
      circular_index = size; // Append
    } else {
      // Wrap index: for size 5, index 6→1, 11→1, -1→4
      circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                        static_cast<int64_t>(size));
    }
  }

  vec->insert(vec->begin() + static_cast<ptrdiff_t>(circular_index), value);

  return ExecutionResult::kNormal;
}

// Specialization for ObjectArray/StringArray/PointerArray (value is void*)
// Uses circular indexing: index wraps around array size
template<>
std::expected<ExecutionResult, std::runtime_error> ArrayInsertAt<void*>(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ArrayInsertAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  void* value = std::get<void*>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);

  // Circular indexing: wrap index to valid range
  // For insert, we allow index == size (append), so we handle it specially
  size_t size = vec->size();
  size_t circular_index = 0;

  if (size == 0) {
    // Empty array: any index becomes 0 (append)
    circular_index = 0;
  } else {
    // Check if index equals size (append case)
    if (index == static_cast<int64_t>(size)) {
      circular_index = size; // Append
    } else {
      // Wrap index: for size 5, index 6→1, 11→1, -1→4
      circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                        static_cast<int64_t>(size));
    }
  }

  vec->insert(vec->begin() + static_cast<ptrdiff_t>(circular_index), value);
  return ExecutionResult::kNormal;
}

// Template helper for array SetAt
// Arguments: object is first, index is second, value is third
// Uses circular indexing: index wraps around array size
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArraySetAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<T>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ArraySetAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  T value = std::get<T>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec = runtime::GetDataPointer<std::vector<T>>(obj_ptr);

  if (vec->empty()) {
    return std::unexpected(std::runtime_error("ArraySetAt: cannot set in empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = vec->size();
  size_t circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                           static_cast<int64_t>(size));

  (*vec)[circular_index] = value;

  return ExecutionResult::kNormal;
}

// Specialization for ObjectArray/StringArray/PointerArray (value is void*)
template<>
std::expected<ExecutionResult, std::runtime_error> ArraySetAt<void*>(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ArraySetAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  void* value = std::get<void*>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);

  if (vec->empty()) {
    return std::unexpected(std::runtime_error("ArraySetAt: cannot set in empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = vec->size();
  size_t circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                           static_cast<int64_t>(size));

  (*vec)[circular_index] = value;
  return ExecutionResult::kNormal;
}

// Template helper for array GetAt
// Arguments: object is first, index is second
// Uses circular indexing: index wraps around array size
template<typename T>
std::expected<ExecutionResult, std::runtime_error> ArrayGetAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayGetAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  const auto* vec = runtime::GetDataPointer<const std::vector<T>>(obj_ptr);

  if (vec->empty()) {
    return std::unexpected(std::runtime_error("ArrayGetAt: cannot get from empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = vec->size();
  size_t circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                           static_cast<int64_t>(size));

  T value = (*vec)[circular_index];
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

// Specialization for ObjectArray/StringArray/PointerArray (returns void*)
template<>
std::expected<ExecutionResult, std::runtime_error> ArrayGetAt<void*>(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ArrayGetAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  const auto* vec = runtime::GetDataPointer<const std::vector<void*>>(obj_ptr);

  if (vec->empty()) {
    return std::unexpected(std::runtime_error("ArrayGetAt: cannot get from empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = vec->size();
  size_t circular_index = ((static_cast<int64_t>(index % static_cast<int64_t>(size)) + static_cast<int64_t>(size)) %
                           static_cast<int64_t>(size));

  void* value = (*vec)[circular_index];
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree

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
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("Bool::IsLess: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const bool* value1 = runtime::GetDataPointer<const bool>(obj1_ptr);
  const bool* value2 = runtime::GetDataPointer<const bool>(obj2_ptr);
  bool is_less = (!*value1 && *value2); // false < true
  data.memory.machine_stack.emplace(is_less);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NullableConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("Nullable::Constructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* value_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  auto* nullable_data = runtime::GetDataPointer<void*>(obj_ptr);
  *nullable_data = value_ptr;
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NullableDestructor(PassedExecutionData& data) {
  return FundamentalTypeDestructor<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> StringToString(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("String::ToString: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  // Return self
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringGetHash(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("String::GetHash: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* str = runtime::GetDataPointer<std::string>(obj_ptr);
  int64_t hash = static_cast<int64_t>(std::hash<std::string>{}(*str));
  data.memory.machine_stack.emplace(hash);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringLength(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("String::Length: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* str = runtime::GetDataPointer<std::string>(obj_ptr);
  auto length = static_cast<int64_t>(str->length());
  data.memory.machine_stack.emplace(length);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringToUtf8Bytes(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("String::ToUtf8Bytes: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* str = runtime::GetDataPointer<std::string>(obj_ptr);

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

  auto byte_array_obj_result = runtime::AllocateObject(*byte_array_vtable,
                                                       static_cast<uint32_t>(vtable_index_result.value()),
                                                       data.memory.object_repository,
                                                       data.allocator);

  if (!byte_array_obj_result.has_value()) {
    return std::unexpected(byte_array_obj_result.error());
  }

  void* byte_array_obj = byte_array_obj_result.value();
  auto* byte_array_data = runtime::GetDataPointer<runtime::ByteArray>(byte_array_obj);
  new (byte_array_data) runtime::ByteArray(str->size());
  std::memcpy(byte_array_data->Data(), str->data(), str->size());
  data.memory.machine_stack.emplace(byte_array_obj);

  return ExecutionResult::kNormal;
}

// String methods

std::expected<ExecutionResult, std::runtime_error> StringCopyConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("String::CopyConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* source_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  const auto* source_string = runtime::GetDataPointer<const std::string>(source_obj);
  auto* string_data = runtime::GetDataPointer<std::string>(obj_ptr);
  new (string_data) std::string(*source_string);
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringDestructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("String::Destructor: invalid argument types"));
  }

  using string_type = std::string;
  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* string_data = runtime::GetDataPointer<std::string>(obj_ptr);
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

std::expected<ExecutionResult, std::runtime_error> IntArrayClear(PassedExecutionData& data) {
  return ArrayClear<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayShrinkToFit(PassedExecutionData& data) {
  return ArrayShrinkToFit<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayReserve(PassedExecutionData& data) {
  return ArrayReserve<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayCapacity(PassedExecutionData& data) {
  return ArrayCapacity<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayAdd(PassedExecutionData& data) {
  return ArrayAdd<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayRemoveAt(PassedExecutionData& data) {
  return ArrayRemoveAt<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayInsertAt(PassedExecutionData& data) {
  return ArrayInsertAt<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArraySetAt(PassedExecutionData& data) {
  return ArraySetAt<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> IntArrayGetAt(PassedExecutionData& data) {
  return ArrayGetAt<int64_t>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayLength(PassedExecutionData& data) {
  return ArrayLength<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayClear(PassedExecutionData& data) {
  return ArrayClear<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayShrinkToFit(PassedExecutionData& data) {
  return ArrayShrinkToFit<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayReserve(PassedExecutionData& data) {
  return ArrayReserve<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayCapacity(PassedExecutionData& data) {
  return ArrayCapacity<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayAdd(PassedExecutionData& data) {
  return ArrayAdd<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayRemoveAt(PassedExecutionData& data) {
  return ArrayRemoveAt<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayInsertAt(PassedExecutionData& data) {
  return ArrayInsertAt<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArraySetAt(PassedExecutionData& data) {
  return ArraySetAt<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> FloatArrayGetAt(PassedExecutionData& data) {
  return ArrayGetAt<double>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayLength(PassedExecutionData& data) {
  return ArrayLength<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayClear(PassedExecutionData& data) {
  return ArrayClear<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayShrinkToFit(PassedExecutionData& data) {
  return ArrayShrinkToFit<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayReserve(PassedExecutionData& data) {
  return ArrayReserve<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayCapacity(PassedExecutionData& data) {
  return ArrayCapacity<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayAdd(PassedExecutionData& data) {
  return ArrayAdd<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayRemoveAt(PassedExecutionData& data) {
  return ArrayRemoveAt<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayInsertAt(PassedExecutionData& data) {
  return ArrayInsertAt<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArraySetAt(PassedExecutionData& data) {
  return ArraySetAt<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> CharArrayGetAt(PassedExecutionData& data) {
  return ArrayGetAt<char>(data);
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayLength(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ByteArrayLength: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const auto* byte_array = runtime::GetDataPointer<const runtime::ByteArray>(obj_ptr);
  auto length = static_cast<int64_t>(byte_array->Size());
  data.memory.machine_stack.emplace(length);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayGetHash(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ByteArrayGetHash: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const auto* byte_array = runtime::GetDataPointer<const runtime::ByteArray>(obj_ptr);
  auto hash = static_cast<int64_t>(byte_array->GetHash());
  data.memory.machine_stack.emplace(hash);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayClear(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ByteArrayClear: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  byte_array->Clear();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayShrinkToFit(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ByteArrayShrinkToFit: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  byte_array->ShrinkToFit();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayReserve(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayReserve: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t capacity = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  byte_array->Reserve(static_cast<size_t>(capacity));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayCapacity(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ByteArrayCapacity: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  const auto* byte_array = runtime::GetDataPointer<const runtime::ByteArray>(obj_ptr);
  auto capacity = static_cast<int64_t>(byte_array->Capacity());
  data.memory.machine_stack.emplace(capacity);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayAdd(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<uint8_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayAdd: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  uint8_t value = std::get<uint8_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  byte_array->Insert(byte_array->Size(), value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayRemoveAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayRemoveAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);

  if (byte_array->Size() == 0) {
    return std::unexpected(std::runtime_error("ByteArrayRemoveAt: cannot remove from empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = byte_array->Size();
  size_t circular_index = ComputeCircularIndex(index, size, false);

  byte_array->Remove(circular_index);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayInsertAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<uint8_t>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ByteArrayInsertAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  uint8_t value = std::get<uint8_t>(data.memory.stack_frames.top().local_variables[2]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);

  // Circular indexing: wrap index to valid range
  // For insert, we allow index == size (append), so we handle it specially
  size_t size = byte_array->Size();
  size_t circular_index = ComputeCircularIndex(index, size, true);

  byte_array->Insert(circular_index, value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArraySetAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<uint8_t>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ByteArraySetAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  uint8_t value = std::get<uint8_t>(data.memory.stack_frames.top().local_variables[2]);
  auto* byte_array = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);

  if (byte_array->Size() == 0) {
    return std::unexpected(std::runtime_error("ByteArraySetAt: cannot set in empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = byte_array->Size();
  size_t circular_index = ComputeCircularIndex(index, size, false);

  (*byte_array)[circular_index] = value;

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayGetAt(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayGetAt: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t index = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  const auto* byte_array = runtime::GetDataPointer<const runtime::ByteArray>(obj_ptr);

  if (byte_array->Size() == 0) {
    return std::unexpected(std::runtime_error("ByteArrayGetAt: cannot get from empty array"));
  }

  // Circular indexing: wrap index to valid range
  size_t size = byte_array->Size();
  size_t circular_index = ComputeCircularIndex(index, size, false);

  uint8_t value = (*byte_array)[circular_index];
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayLength(PassedExecutionData& data) {
  return ArrayLength<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayClear(PassedExecutionData& data) {
  return ArrayClear<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayShrinkToFit(PassedExecutionData& data) {
  return ArrayShrinkToFit<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayReserve(PassedExecutionData& data) {
  return ArrayReserve<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayCapacity(PassedExecutionData& data) {
  return ArrayCapacity<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayAdd(PassedExecutionData& data) {
  return ArrayAdd<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayRemoveAt(PassedExecutionData& data) {
  return ArrayRemoveAt<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayInsertAt(PassedExecutionData& data) {
  return ArrayInsertAt<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArraySetAt(PassedExecutionData& data) {
  return ArraySetAt<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> BoolArrayGetAt(PassedExecutionData& data) {
  return ArrayGetAt<bool>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayLength(PassedExecutionData& data) {
  return ArrayLength<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayGetHash(PassedExecutionData& data) {
  return ArrayGetHash<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayClear(PassedExecutionData& data) {
  return ArrayClear<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayShrinkToFit(PassedExecutionData& data) {
  return ArrayShrinkToFit<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayReserve(PassedExecutionData& data) {
  return ArrayReserve<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayCapacity(PassedExecutionData& data) {
  return ArrayCapacity<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayAdd(PassedExecutionData& data) {
  return ArrayAdd<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayRemoveAt(PassedExecutionData& data) {
  return ArrayRemoveAt<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayInsertAt(PassedExecutionData& data) {
  return ArrayInsertAt<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArraySetAt(PassedExecutionData& data) {
  return ArraySetAt<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> ObjectArrayGetAt(PassedExecutionData& data) {
  return ArrayGetAt<void*>(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayLength(PassedExecutionData& data) {
  return ObjectArrayLength(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayGetHash(PassedExecutionData& data) {
  return ObjectArrayGetHash(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayClear(PassedExecutionData& data) {
  return ObjectArrayClear(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayShrinkToFit(PassedExecutionData& data) {
  return ObjectArrayShrinkToFit(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayReserve(PassedExecutionData& data) {
  return ObjectArrayReserve(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayCapacity(PassedExecutionData& data) {
  return ObjectArrayCapacity(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayAdd(PassedExecutionData& data) {
  return ObjectArrayAdd(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayRemoveAt(PassedExecutionData& data) {
  return ObjectArrayRemoveAt(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayInsertAt(PassedExecutionData& data) {
  return ObjectArrayInsertAt(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArraySetAt(PassedExecutionData& data) {
  return ObjectArraySetAt(data);
}

std::expected<ExecutionResult, std::runtime_error> StringArrayGetAt(PassedExecutionData& data) {
  return ObjectArrayGetAt(data);
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

std::expected<ExecutionResult, std::runtime_error> PointerArrayClear(PassedExecutionData& data) {
  return ObjectArrayClear(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayShrinkToFit(PassedExecutionData& data) {
  return ObjectArrayShrinkToFit(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayReserve(PassedExecutionData& data) {
  return ObjectArrayReserve(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayCapacity(PassedExecutionData& data) {
  return ObjectArrayCapacity(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayAdd(PassedExecutionData& data) {
  return ObjectArrayAdd(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayRemoveAt(PassedExecutionData& data) {
  return ObjectArrayRemoveAt(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayInsertAt(PassedExecutionData& data) {
  return ObjectArrayInsertAt(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArraySetAt(PassedExecutionData& data) {
  return ObjectArraySetAt(data);
}

std::expected<ExecutionResult, std::runtime_error> PointerArrayGetAt(PassedExecutionData& data) {
  return ObjectArrayGetAt(data);
}

// File methods
// Arguments: file is first, path is second, mode is third
std::expected<ExecutionResult, std::runtime_error> FileOpen(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("File::Open: invalid argument types"));
  }

  void* file_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* path_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  void* mode_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[2]);
  auto* path = runtime::GetDataPointer<std::string>(path_obj);
  auto* mode = runtime::GetDataPointer<std::string>(mode_obj);
  auto* file = runtime::GetDataPointer<std::fstream>(file_obj);

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
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::Close: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (file->is_open()) {
    file->close();
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileIsOpen(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::IsOpen: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);
  bool is_open = file->is_open();
  data.memory.machine_stack.emplace(is_open);

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
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<uint8_t>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ByteArrayConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t size = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  uint8_t default_value = std::get<uint8_t>(data.memory.stack_frames.top().local_variables[2]);
  auto* byte_array_data = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  new (byte_array_data) runtime::ByteArray(static_cast<size_t>(size));
  if (size > 0) {
    std::memset(byte_array_data->Data(), default_value, static_cast<size_t>(size));
  }
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayCopyConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayCopyConstructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* source_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  const auto* source_byte_array = runtime::GetDataPointer<const runtime::ByteArray>(source_obj);
  auto* byte_array_data = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  new (byte_array_data) runtime::ByteArray(*source_byte_array);
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayDestructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("ByteArrayDestructor: invalid argument types"));
  }

  using byte_array_type = runtime::ByteArray;
  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* byte_array_data = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);
  byte_array_data->~byte_array_type();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayEquals(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayEquals: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const auto* byte_array1 = runtime::GetDataPointer<const runtime::ByteArray>(obj1_ptr);
  const auto* byte_array2 = runtime::GetDataPointer<const runtime::ByteArray>(obj2_ptr);
  bool equals = (*byte_array1 == *byte_array2);
  data.memory.machine_stack.emplace(equals);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteArrayIsLess(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArrayIsLess: invalid argument types"));
  }

  void* obj1_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* obj2_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Check if types match - if not, return false
  if (!AreSameType(obj1_ptr, obj2_ptr)) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }

  const auto* byte_array1 = runtime::GetDataPointer<const runtime::ByteArray>(obj1_ptr);
  const auto* byte_array2 = runtime::GetDataPointer<const runtime::ByteArray>(obj2_ptr);
  bool is_less = (*byte_array1 < *byte_array2);
  data.memory.machine_stack.emplace(is_less);

  return ExecutionResult::kNormal;
}

// ByteArray constructor from Object (creates a view)
// Arguments: object (this) is first, source object is second
std::expected<ExecutionResult, std::runtime_error> ByteArrayFromObject(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("ByteArray::FromObject: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* source_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  // Get ObjectDescriptor from the source object
  const auto* descriptor = reinterpret_cast<const runtime::ObjectDescriptor*>(source_obj);
  uint32_t vtable_index = descriptor->vtable_index;

  // Get the vtable from the repository
  auto vtable_result = data.virtual_table_repository.GetByIndex(vtable_index);
  if (!vtable_result.has_value()) {
    return std::unexpected(
        std::runtime_error("ByteArray::FromObject: vtable not found for index " + std::to_string(vtable_index)));
  }

  const runtime::VirtualTable* vtable = vtable_result.value();
  size_t object_size = vtable->GetSize();

  auto* byte_array_data = runtime::GetDataPointer<runtime::ByteArray>(obj_ptr);

  // Create a view of the entire source object (including ObjectDescriptor)
  new (byte_array_data) runtime::ByteArray(source_obj, object_size);
  data.memory.machine_stack.emplace(obj_ptr);

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
// Arguments: object (this) is first, size is second, default_value is third
std::expected<ExecutionResult, std::runtime_error> ObjectArrayConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[2])) {
    return std::unexpected(std::runtime_error("ObjectArray::Constructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t size = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  void* default_value = std::get<void*>(data.memory.stack_frames.top().local_variables[2]);
  auto* vec_data = runtime::GetDataPointer<std::vector<void*>>(obj_ptr);
  new (vec_data) std::vector<void*>(static_cast<size_t>(size), default_value);
  data.memory.machine_stack.emplace(obj_ptr);

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

// Arguments: file is first, size is second
std::expected<ExecutionResult, std::runtime_error> FileRead(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("File::Read: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t size = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Read: file is not open"));
  }

  // Read bytes
  std::vector<uint8_t> buffer(static_cast<size_t>(size));
  file->read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
  auto bytes_read = static_cast<size_t>(file->gcount());
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

  auto byte_array_obj_result = runtime::AllocateObject(*byte_array_vtable,
                                                       static_cast<uint32_t>(vtable_index_result.value()),
                                                       data.memory.object_repository,
                                                       data.allocator);

  if (!byte_array_obj_result.has_value()) {
    return std::unexpected(byte_array_obj_result.error());
  }

  void* byte_array_obj = byte_array_obj_result.value();
  auto* byte_array_data = runtime::GetDataPointer<runtime::ByteArray>(byte_array_obj);
  new (byte_array_data) runtime::ByteArray(buffer.size());
  std::memcpy(byte_array_data->Data(), buffer.data(), buffer.size());
  data.memory.machine_stack.emplace(byte_array_obj);

  return ExecutionResult::kNormal;
}

// Arguments: file is first, byte_array is second
std::expected<ExecutionResult, std::runtime_error> FileWrite(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("File::Write: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* byte_array_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);
  const auto* byte_array = runtime::GetDataPointer<const runtime::ByteArray>(byte_array_obj);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Write: file is not open"));
  }

  // Write bytes
  file->write(reinterpret_cast<const char*>(byte_array->Data()), static_cast<std::streamsize>(byte_array->Size()));

  if (file->fail()) {
    return std::unexpected(std::runtime_error("File::Write: write failed"));
  }

  auto bytes_written = static_cast<int64_t>(byte_array->Size());
  data.memory.machine_stack.emplace(bytes_written);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileReadLine(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::ReadLine: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);

  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

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

  auto string_obj_result = runtime::AllocateObject(*string_vtable,
                                                   static_cast<uint32_t>(vtable_index_result.value()),
                                                   data.memory.object_repository,
                                                   data.allocator);

  if (!string_obj_result.has_value()) {
    return std::unexpected(string_obj_result.error());
  }

  void* string_obj = string_obj_result.value();
  auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
  new (string_data) std::string(line);
  data.memory.machine_stack.emplace(string_obj);
  return ExecutionResult::kNormal;
}

// Arguments: file is first, line is second
std::expected<ExecutionResult, std::runtime_error> FileWriteLine(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("File::WriteLine: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  void* line_obj = std::get<void*>(data.memory.stack_frames.top().local_variables[1]);

  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);
  auto* line = runtime::GetDataPointer<std::string>(line_obj);

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

// Arguments: file is first, position is second
std::expected<ExecutionResult, std::runtime_error> FileSeek(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0]) ||
      !std::holds_alternative<int64_t>(data.memory.stack_frames.top().local_variables[1])) {
    return std::unexpected(std::runtime_error("File::Seek: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  int64_t position = std::get<int64_t>(data.memory.stack_frames.top().local_variables[1]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

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
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::Tell: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Tell: file is not open"));
  }

  std::streampos pos = file->tellg();
  auto position = static_cast<int64_t>(pos);
  data.memory.machine_stack.emplace(position);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileEof(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::Eof: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* file = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (!file->is_open()) {
    return std::unexpected(std::runtime_error("File::Eof: file is not open"));
  }

  bool eof = file->eof();
  data.memory.machine_stack.emplace(eof);

  return ExecutionResult::kNormal;
}

// File methods (constructor, destructor)
std::expected<ExecutionResult, std::runtime_error> FileConstructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::Constructor: invalid argument types"));
  }

  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* file_data = runtime::GetDataPointer<std::fstream>(obj_ptr);
  new (file_data) std::fstream();
  data.memory.machine_stack.emplace(obj_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FileDestructor(PassedExecutionData& data) {
  if (!std::holds_alternative<void*>(data.memory.stack_frames.top().local_variables[0])) {
    return std::unexpected(std::runtime_error("File::Destructor: invalid argument types"));
  }

  using fstream_type = std::fstream;
  void* obj_ptr = std::get<void*>(data.memory.stack_frames.top().local_variables[0]);
  auto* file_data = runtime::GetDataPointer<std::fstream>(obj_ptr);

  if (file_data->is_open()) {
    file_data->close();
  }

  file_data->~fstream_type();

  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree
