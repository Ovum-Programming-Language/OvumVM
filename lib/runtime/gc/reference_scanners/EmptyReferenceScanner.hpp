#ifndef RUNTIME_EMPTYREFERENCESCANNER_HPP
#define RUNTIME_EMPTYREFERENCESCANNER_HPP

#include "IReferenceScanner.hpp"

namespace ovum::vm::runtime {

class EmptyReferenceScanner : public IReferenceScanner {
public:
  void Scan(void*, const ReferenceVisitor&) const override {
    // No references to scan
  }
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_EMPTYREFERENCESCANNER_HPP
