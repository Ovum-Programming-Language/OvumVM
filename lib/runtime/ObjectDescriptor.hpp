#ifndef RUNTIME_OBJECTDESCRIPTOR_HPP
#define RUNTIME_OBJECTDESCRIPTOR_HPP

#include <cstdint>

namespace ovum::vm::runtime {

struct ObjectDescriptor {
  uint32_t vtable_index;
  uint32_t badge;
  size_t repo_index;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_OBJECTDESCRIPTOR_HPP
