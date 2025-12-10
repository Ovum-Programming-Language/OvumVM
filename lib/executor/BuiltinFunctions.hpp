#ifndef EXECUTOR_BUILTINFUNCTIONS_HPP
#define EXECUTOR_BUILTINFUNCTIONS_HPP

#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/ObjectRepository.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::vm::runtime {

// Helper to get data pointer after ObjectDescriptor
template<typename T>
T* GetDataPointer(void* object_ptr) {
  return reinterpret_cast<T*>(reinterpret_cast<char*>(object_ptr) + sizeof(ObjectDescriptor));
}

template<typename T>
const T* GetDataPointer(const void* object_ptr) {
  return reinterpret_cast<const T*>(reinterpret_cast<const char*>(object_ptr) + sizeof(ObjectDescriptor));
}

// Helper to hash vector using algorithm from PureFunction.hpp
template<typename T>
int64_t HashVector(const std::vector<T>& vec) {
  static constexpr size_t kHashMultiplier = 0x9e3779b9;
  static constexpr size_t kHashShift = 6;

  size_t seed = 0;
  for (const T& value : vec) {
    seed ^= std::hash<T>{}(value) + kHashMultiplier + (seed << kHashShift) + (seed >> kHashShift);
  }
  return static_cast<int64_t>(seed);
}

// Allocation helper: allocates memory for an object and initializes ObjectDescriptor
template<typename Allocator = std::allocator<char>>
std::expected<void*, std::runtime_error> AllocateObject(const VirtualTable& vtable,
                                                        uint32_t vtable_index,
                                                        ObjectRepository& object_repository,
                                                        Allocator&& allocator = Allocator{}) {
  const size_t size = vtable.GetSize();
  char* memory = allocator.allocate(size);
  if (memory == nullptr) {
    return std::unexpected(std::runtime_error("AllocateObject: failed to allocate memory"));
  }

  // Initialize ObjectDescriptor at the first 8 bytes
  ObjectDescriptor* descriptor = reinterpret_cast<ObjectDescriptor*>(memory);
  descriptor->vtable_index = vtable_index;
  descriptor->badge = 0;

  // Zero-initialize the rest of the memory
  std::memset(memory + sizeof(ObjectDescriptor), 0, size - sizeof(ObjectDescriptor));

  // Add to ObjectRepository
  auto add_result = object_repository.Add(descriptor);
  if (!add_result.has_value()) {
    allocator.deallocate(memory, size);
    return std::unexpected(std::runtime_error(std::string("AllocateObject: failed to add object to repository: ") +
                                              add_result.error().what()));
  }

  return memory;
}

} // namespace ovum::vm::runtime

namespace ovum::vm::execution_tree {

// Int methods
std::expected<ExecutionResult, std::runtime_error> IntConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntGetHash(PassedExecutionData& data);

// Float methods
std::expected<ExecutionResult, std::runtime_error> FloatConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatGetHash(PassedExecutionData& data);

// Char methods
std::expected<ExecutionResult, std::runtime_error> CharConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharGetHash(PassedExecutionData& data);

// Byte methods
std::expected<ExecutionResult, std::runtime_error> ByteConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteGetHash(PassedExecutionData& data);

// Bool methods
std::expected<ExecutionResult, std::runtime_error> BoolConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolGetHash(PassedExecutionData& data);

// Nullable methods
std::expected<ExecutionResult, std::runtime_error> NullableConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> NullableDestructor(PassedExecutionData& data);

// String methods
std::expected<ExecutionResult, std::runtime_error> StringCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringToUtf8Bytes(PassedExecutionData& data);

// Array methods
std::expected<ExecutionResult, std::runtime_error> IntArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntArrayGetAt(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> FloatArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatArrayGetAt(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> CharArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharArrayGetAt(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> ByteArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteArrayGetAt(PassedExecutionData& data);

// ByteArray constructor from Object (creates a view)
std::expected<ExecutionResult, std::runtime_error> ByteArrayFromObject(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> BoolArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolArrayGetAt(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> ObjectArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ObjectArrayGetAt(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> StringArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringArrayGetAt(PassedExecutionData& data);

// Pointer methods
std::expected<ExecutionResult, std::runtime_error> PointerConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerGetHash(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> PointerArrayConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayCopyConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayEquals(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayIsLess(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayGetHash(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayClear(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayShrinkToFit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayReserve(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayCapacity(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayRemoveAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayInsertAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArraySetAt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PointerArrayGetAt(PassedExecutionData& data);

// File methods
std::expected<ExecutionResult, std::runtime_error> FileConstructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileDestructor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileOpen(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileClose(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileIsOpen(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileRead(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileWrite(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileReadLine(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileWriteLine(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileSeek(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileTell(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileEof(PassedExecutionData& data);

} // namespace ovum::vm::execution_tree

#endif // EXECUTOR_BUILTINFUNCTIONS_HPP
