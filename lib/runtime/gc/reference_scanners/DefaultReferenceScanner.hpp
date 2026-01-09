#ifndef RUNTIME_DEFAULTREFERENCESCANNER_HPP
#define RUNTIME_DEFAULTREFERENCESCANNER_HPP

#include <vector>

#include "lib/runtime/FieldInfo.hpp"
#include "lib/runtime/VirtualTable.hpp"

#include "IReferenceScanner.hpp"

namespace ovum::vm::runtime {

class DefaultReferenceScanner : public IReferenceScanner {
public:
  explicit DefaultReferenceScanner(const VirtualTable& vt);

  void Scan(void* obj, const ReferenceVisitor& visitor) const override;

private:
  std::vector<FieldInfo> reference_fields_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_DEFAULTREFERENCESCANNER_HPP
