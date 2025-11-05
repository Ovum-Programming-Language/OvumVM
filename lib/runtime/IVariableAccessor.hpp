#ifndef RUNTIME_IVARIABLEACCESSOR_HPP
#define RUNTIME_IVARIABLEACCESSOR_HPP

#include <expected>
#include <stdexcept>

#include "Variable.hpp"

namespace ovum::vm::runtime {

class IVariableAccessor { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IVariableAccessor() = default;

  virtual Variable GetVariable(void* value_ptr) const = 0;
  virtual std::expected<void, std::runtime_error> WriteVariable(void* value_ptr, const Variable& variable) const = 0;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_IVARIABLEACCESSOR_HPP
