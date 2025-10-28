#ifndef RUNTIME_VARIABLE_HPP
#define RUNTIME_VARIABLE_HPP

#include <cstdint>
#include <variant>

namespace ovum::vm::runtime {

/**
 * A variant of all possible variables in the Ovum runtime.
 * void* is used to store pointers to any Ovum object.
 */

using Variable = std::variant<int64_t, double, bool, char, uint8_t, void*>;

template<typename T>
concept VariableMemberType = std::is_same_v<T, int64_t> || std::is_same_v<T, double> || std::is_same_v<T, bool> ||
                             std::is_same_v<T, char> || std::is_same_v<T, uint8_t> || std::is_same_v<T, void*>;

} // namespace ovum::vm::runtime

#endif // RUNTIME_VARIABLE_HPP
