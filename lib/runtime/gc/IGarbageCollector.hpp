#ifndef RUNTIME_GARBAGECOLLECTOR_HPP
#define RUNTIME_GARBAGECOLLECTOR_HPP

#include <expected>
#include <stdexcept>

namespace ovum::vm::execution_tree {
struct PassedExecutionData;
} // namespace ovum::vm::execution_tree

namespace ovum::vm::runtime {

constexpr uint32_t kMarkBit = 1U;

class IGarbageCollector { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  virtual ~IGarbageCollector() = default;
  virtual std::expected<void, std::runtime_error> Collect(execution_tree::PassedExecutionData& data) = 0;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_GARBAGECOLLECTOR_HPP
