#include "MemoryManager.hpp"

#include <utility>
#include <vector>

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/gc/MarkAndSweepGC.hpp"

#include "ObjectDescriptor.hpp"

namespace ovum::vm::runtime {

MemoryManager::MemoryManager() : gc_threshold_(kDefaultMaxObjects) {
  gc_ = std::make_unique<MarkAndSweepGC>();
}

std::expected<void*, std::runtime_error> MemoryManager::AllocateObject(const VirtualTable& vtable,
                                                                       uint32_t vtable_index,
                                                                       execution_tree::PassedExecutionData& data) {
  const size_t total_size = vtable.GetSize();
  char* raw_memory = allocator_.allocate(total_size);
  if (!raw_memory) {
    return std::unexpected(std::runtime_error("MemoryManager: Allocation failed - out of memory"));
  }

  auto* descriptor = reinterpret_cast<ObjectDescriptor*>(raw_memory);
  descriptor->vtable_index = vtable_index;
  descriptor->badge = 0;

  auto add_result = repo_.Add(descriptor);
  if (!add_result.has_value()) {
    allocator_.deallocate(raw_memory, total_size);
    return std::unexpected(add_result.error());
  }

  if (repo_.GetCount() > gc_threshold_) {
    auto collect_res = CollectGarbage(data);

    if (!collect_res.has_value()) {
      return std::unexpected(collect_res.error());
    }
  }

  return reinterpret_cast<void*>(descriptor);
}

std::expected<void, std::runtime_error> MemoryManager::DeallocateObject(void* obj,
                                                                        execution_tree::PassedExecutionData& data) {
  if (!obj) {
    return std::unexpected(std::runtime_error("DeallocateObject: Null object pointer"));
  }

  auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

  auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
  if (!vt_res.has_value()) {
    return std::unexpected(std::runtime_error("DeallocateObject: Virtual table not found for index " +
                                              std::to_string(desc->vtable_index)));
  }

  std::string object_type = vt_res.value()->GetName();

  const VirtualTable* vt = vt_res.value();

  auto dtor_id_res = vt->GetRealFunctionId("_destructor_<M>");

  if (!dtor_id_res.has_value()) {
    return std::unexpected(std::runtime_error("DeallocateObject: Destructor not found for class " + object_type));
  }

  auto func_res = data.function_repository.GetById(dtor_id_res.value());

  if (!func_res.has_value()) {
    return std::unexpected(
        std::runtime_error("DeallocateObject: Destructor function not found for class " + object_type));
  }

  runtime::StackFrame frame = {.function_name = "Object deallocation"};
  data.memory.machine_stack.emplace(obj);
  data.memory.stack_frames.push(std::move(frame));
  auto exec_res = func_res.value()->Execute(data);
  data.memory.stack_frames.pop();

  const size_t total_size = vt->GetSize();
  char* raw = reinterpret_cast<char*>(obj);

  // Remove from repository BEFORE deallocating memory
  auto remove_res = repo_.Remove(desc);
  if (!remove_res.has_value()) {
    return std::unexpected(remove_res.error());
  }

  // Now safe to deallocate memory
  allocator_.deallocate(raw, total_size);

  if (!exec_res.has_value()) {
    return std::unexpected(exec_res.error());
  }

  return {};
}

std::expected<void, std::runtime_error> MemoryManager::Clear(execution_tree::PassedExecutionData& data) {
  std::vector<void*> objects_to_clear;
  objects_to_clear.reserve(repo_.GetCount());
  std::expected<void, std::runtime_error> clear_res = {};

  repo_.ForAll([&objects_to_clear](void* obj) { objects_to_clear.push_back(obj); });

  for (void* obj : objects_to_clear) {
    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

    auto dealloc_res = DeallocateObject(obj, data);
    if (!dealloc_res.has_value()) {
      // If deallocation fails, try to remove from repository and deallocate manually
      auto remove_res = repo_.Remove(desc);
      if (remove_res.has_value()) {
        // Try to get vtable to deallocate memory
        auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
        if (vt_res.has_value()) {
          const size_t total_size = vt_res.value()->GetSize();
          char* raw = reinterpret_cast<char*>(obj);
          allocator_.deallocate(raw, total_size);
        }
      }
      clear_res = dealloc_res; // Save the error
    }
  }

  repo_.Clear();

  return clear_res;
}

std::expected<void, std::runtime_error> MemoryManager::CollectGarbage(execution_tree::PassedExecutionData& data) {
  if (!gc_) {
    return std::unexpected(std::runtime_error("MemoryManager: No GC configured"));
  }

  return gc_.value()->Collect(data);
}

const ObjectRepository& MemoryManager::GetRepository() const {
  return repo_;
}

} // namespace ovum::vm::runtime
