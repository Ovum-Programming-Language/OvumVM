
#include "DefaultReferenceScanner.hpp"

#include <vector>

#include "lib/runtime/FieldInfo.hpp"
#include "lib/runtime/Variable.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::vm::runtime {

DefaultReferenceScanner::DefaultReferenceScanner(const VirtualTable& vt) {
  for (size_t i = 0; i < vt.GetFieldCount(); ++i) {
    if (vt.IsFieldReferenceType(i)) {
      reference_fields_.emplace_back(vt.GetFieldOffset(i), vt.GetFieldAccessor(i));
    }
  }
}

void DefaultReferenceScanner::Scan(void* obj, const ReferenceVisitor& visitor) const {
  for (const FieldInfo& field : reference_fields_) {
    Variable var_res = field.variable_accessor->GetVariable(reinterpret_cast<char*>(obj) + field.offset);

    if (!std::holds_alternative<void*>(var_res)) {
      continue;
    }

    visitor(std::get<void*>(var_res));
  }
}

} // namespace ovum::vm::runtime
