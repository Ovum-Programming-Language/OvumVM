
#include "DefaultReferenceScanner.hpp"

#include <vector>

#include "lib/runtime/Variable.hpp"
#include "lib/runtime/FieldInfo.hpp"

namespace ovum::vm::runtime {

void DefaultReferenceScanner::Scan(void* obj, const std::vector<FieldInfo>& fields, const ReferenceVisitor& visitor) const {
  for (const FieldInfo& field : fields) {
    const Variable& var = field.variable_accessor->GetVariable(reinterpret_cast<char*>(obj) + field.offset);

    if (!std::holds_alternative<void*>(var)) {
      continue;
    }

    visitor(std::get<void*>(var));
  }
}

} // namespace ovum::vm::runtime
