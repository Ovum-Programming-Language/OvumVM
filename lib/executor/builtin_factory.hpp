#ifndef EXECUTOR_BUILTIN_FACTORY_HPP
#define EXECUTOR_BUILTIN_FACTORY_HPP

#include <expected>
#include <stdexcept>

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

/**
 * Registers all builtin class virtual tables in the repository.
 * Builtin types: Int, Float, Char, Byte, Bool, Nullable, String,
 * IntArray, FloatArray, CharArray, ByteArray, BoolArray, ObjectArray,
 * StringArray, PointerArray, Pointer, File.
 * @param repository The virtual table repository to populate.
 * @return Error if registration fails.
 */
std::expected<void, std::runtime_error> RegisterBuiltinVirtualTables(VirtualTableRepository& repository);

} // namespace ovum::vm::runtime

namespace ovum::vm::execution_tree {

/**
 * Registers all builtin method functions in the repository.
 * Includes methods like ToString, Length, ToUtf8Bytes, etc.
 * @param repository The function repository to populate.
 * @return Error if registration fails.
 */
std::expected<void, std::runtime_error> RegisterBuiltinFunctions(FunctionRepository& repository);

} // namespace ovum::vm::execution_tree

#endif // EXECUTOR_BUILTIN_FACTORY_HPP
