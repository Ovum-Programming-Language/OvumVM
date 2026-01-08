#ifndef RUNTIME_ARRAYREFERENCESCANNER_HPP
#define RUNTIME_ARRAYREFERENCESCANNER_HPP

#include <type_traits>
#include <vector>

#include "lib/runtime/ObjectDescriptor.hpp"

#include "IReferenceScanner.hpp"

namespace ovum::vm::runtime {

template<typename T>
class ArrayReferenceScanner : public IReferenceScanner {
public:
  void Scan(void* obj, const ReferenceVisitor& visitor) const override {
    const char* base = reinterpret_cast<const char*>(obj) + sizeof(ObjectDescriptor);
    const std::vector<T>& vec = *reinterpret_cast<const std::vector<T>*>(base);

    if constexpr (std::is_pointer_v<T>) {
      for (T p : vec) {
        if (p) {
          visitor(reinterpret_cast<void*>(p));
        }
      }
    } else {
      (void) visitor;
    }
  }
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_ARRAYREFERENCESCANNER_HPP
