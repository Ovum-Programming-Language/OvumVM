#ifndef RUNTIME_ARRAYREFERENCESCANNER_HPP
#define RUNTIME_ARRAYREFERENCESCANNER_HPP

#include "IReferenceScanner.hpp"

namespace ovum::vm::runtime {

class ArrayReferenceScanner : public IReferenceScanner {
public:
  void Scan(void* obj, const std::vector<FieldInfo>& fields, const ReferenceVisitor& visitor) const override;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_ARRAYREFERENCESCANNER_HPP
