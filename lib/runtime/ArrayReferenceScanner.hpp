#ifndef RUNTIME_ARRAYREFERENCESCANNER_HPP
#define RUNTIME_ARRAYREFERENCESCANNER_HPP

#include <vector>

#include "IReferenceScanner.hpp"
#include "ObjectDescriptor.hpp"  // Для GetDataPointer

namespace ovum::vm::runtime {

template <typename T>
class ArrayReferenceScanner : public IReferenceScanner {
public:
  void Scan(void* obj, const ReferenceVisitor& visitor) const override {
    const auto& vec = *GetDataPointer<const std::vector<T>>(obj);
    for (auto p : vec) {
      if (p) {
        visitor(p);
      }
    }
  }
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_ARRAYREFERENCESCANNER_HPP
