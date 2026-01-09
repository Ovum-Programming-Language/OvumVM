#ifndef RUNTIME_DEFAULTREFERENCESCANNER_HPP
#define RUNTIME_DEFAULTREFERENCESCANNER_HPP

#include "IReferenceScanner.hpp"

namespace ovum::vm::runtime {

class DefaultReferenceScanner : public IReferenceScanner {
public:
  void Scan(void* obj, const std::vector<FieldInfo>& fields, const ReferenceVisitor& visitor) const override;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_DEFAULTREFERENCESCANNER_HPP
