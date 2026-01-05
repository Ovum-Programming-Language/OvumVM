#ifndef RUNTIME_DEFAULTREFERENCESCANNER_HPP
#define RUNTIME_DEFAULTREFERENCESCANNER_HPP

#include "IReferenceScanner.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::vm::runtime {

class DefaultReferenceScanner : public IReferenceScanner {
public:
  explicit DefaultReferenceScanner(const VirtualTable* vt) : vt_(vt) {}

  void Scan(void* obj, const ReferenceVisitor& visitor) const override {
    for (size_t i = 0; i < vt_->GetFieldCount(); ++i) {
      if (vt_->IsFieldReferenceType(i)) {
        auto var_res = vt_->GetVariableByIndex(obj, i);
        if (var_res.has_value() && std::holds_alternative<void*>(var_res.value())) {
          void* ptr = std::get<void*>(var_res.value());
          if (ptr) {
            visitor(ptr);
          }
        }
      }
    }
  }

private:
  const VirtualTable* vt_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_DEFAULTREFERENCESCANNER_HPP
