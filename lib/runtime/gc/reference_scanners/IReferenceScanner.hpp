#ifndef RUNTIME_IREFERENCESCANNER_HPP
#define RUNTIME_IREFERENCESCANNER_HPP

#include <functional>

namespace ovum::vm::runtime {

using ReferenceVisitor = std::function<void(void*)>;

class IReferenceScanner { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IReferenceScanner() = default;
  virtual void Scan(void* obj, const ReferenceVisitor& visitor) const = 0;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_IREFERENCESCANNER_HPP
