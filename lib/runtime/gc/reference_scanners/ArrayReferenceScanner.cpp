#include "ArrayReferenceScanner.hpp"

#include <vector>

#include "lib/runtime/ObjectDescriptor.hpp"

namespace ovum::vm::runtime {

void ArrayReferenceScanner::Scan(void* obj, const std::vector<FieldInfo>&, const ReferenceVisitor& visitor) const {
  const char* base = reinterpret_cast<const char*>(obj) + sizeof(ObjectDescriptor);
  const std::vector<void*>& vec = *reinterpret_cast<const std::vector<void*>*>(base);

  for (void* p : vec) {
    visitor(p);
  }
}

} // namespace ovum::vm::runtime
