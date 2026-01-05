#ifndef RUNTIME_MEMORYMANAGER_HPP
#define RUNTIME_MEMORYMANAGER_HPP

#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>

#include "IGarbageCollector.hpp"
#include "ObjectRepository.hpp"
#include "VirtualTable.hpp"

namespace ovum::vm::execution_tree { struct PassedExecutionData; }  // Forward declaration

namespace ovum::vm::runtime {

constexpr size_t kDefaultMaxObjects = 10000;  // Default threshold for auto-GC

class MemoryManager {
public:
  MemoryManager();

  std::expected<void*, std::runtime_error> AllocateObject(const VirtualTable& vtable,
                                                          uint32_t vtable_index,
                                                          execution_tree::PassedExecutionData& data);
  std::expected<void, std::runtime_error> DeallocateObject(void* obj,
                                                           execution_tree::PassedExecutionData& data);
  std::expected<void, std::runtime_error> CollectGarbage(execution_tree::PassedExecutionData& data);
  std::expected<void, std::runtime_error> Clear(execution_tree::PassedExecutionData& data);

  [[nodiscard]] const ObjectRepository& GetRepository() const;

private:
  ObjectRepository repo_;
  std::allocator<char> allocator_;
  std::optional<std::unique_ptr<IGarbageCollector>> gc_;
  size_t max_objects_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_MEMORYMANAGER_HPP
