#ifndef RUNTIME_FIELDINFO_HPP
#define RUNTIME_FIELDINFO_HPP

#include <cstdint>
#include <memory>

#include "IVariableAccessor.hpp"

namespace ovum::vm::runtime {

struct FieldInfo {
  int64_t offset;
  std::shared_ptr<IVariableAccessor> variable_accessor;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_FIELDINFO_HPP
