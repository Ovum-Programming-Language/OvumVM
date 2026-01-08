#ifndef RUNTIME_DEFAULTREFERENCESCANNER_HPP
#define RUNTIME_DEFAULTREFERENCESCANNER_HPP

#include <vector>

#include "lib/runtime/FieldInfo.hpp"
#include "lib/runtime/Variable.hpp"
#include "lib/runtime/VirtualTable.hpp"

#include "IReferenceScanner.hpp"

namespace ovum::vm::runtime {

class DefaultReferenceScanner : public IReferenceScanner {
public:
  explicit DefaultReferenceScanner(const VirtualTable& vt) {
    for (size_t i = 0; i < vt.GetFieldCount(); ++i) {
      if (vt.IsFieldReferenceType(i)) {
        reference_fields_.emplace_back(vt.GetFieldOffset(i), vt.GetFieldAccessor(i));
      }
    }
  }

  void Scan(void* obj, const ReferenceVisitor& visitor) const override {
    for (const FieldInfo& field : reference_fields_) {
      Variable var_res = field.variable_accessor->GetVariable(reinterpret_cast<char*>(obj) + field.offset);

      if (std::holds_alternative<void*>(var_res)) {
        void* ptr = std::get<void*>(var_res);

        if (ptr) {
          visitor(ptr);
        }
      }
    }
  }

private:
  std::vector<FieldInfo> reference_fields_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_DEFAULTREFERENCESCANNER_HPP
