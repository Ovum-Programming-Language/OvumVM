#include "MemoryManager.hpp"

#include <utility>

#include "ObjectDescriptor.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/gc/MarkAndSweepGC.hpp"

namespace ovum::vm::runtime {

MemoryManager::MemoryManager() : max_objects_(kDefaultMaxObjects) {
  gc_ = std::make_unique<MarkAndSweepGC>();  // Assume enabled by default
}

std::expected<void*, std::runtime_error> MemoryManager::AllocateObject(const VirtualTable& vtable,
                                                                       uint32_t vtable_index,
                                                                       execution_tree::PassedExecutionData& data) {
  const size_t size = sizeof(ObjectDescriptor) + vtable.GetSize();
  char* raw_memory = allocator_.allocate(size);
  if (!raw_memory) {
    return std::unexpected(std::runtime_error("MemoryManager: Allocation failed"));
  }

  auto* descriptor = reinterpret_cast<ObjectDescriptor*>(raw_memory);
  descriptor->vtable_index = vtable_index;
  descriptor->badge = 0;  // Clear mark bit

  auto add_result = repo_.Add(descriptor);
  if (!add_result.has_value()) {
    allocator_.deallocate(raw_memory, size);
    return std::unexpected(add_result.error());
  }

  // Авто-GC если превышен threshold
  if (repo_.GetCount() > max_objects_) {
    auto gc_result = CollectGarbage(data);
    if (!gc_result.has_value()) {
      // Log error, but continue
    }
  }

  return reinterpret_cast<void*>(descriptor);
}

std::expected<void, std::runtime_error> MemoryManager::DeallocateObject(void* obj,
                                                                        execution_tree::PassedExecutionData& data) {
  if (!obj) {
    return std::unexpected(std::runtime_error("DeallocateObject: Null object"));
  }

  // Найти индекс в repo
  auto index = static_cast<size_t>(-1);
  for (size_t i = 0; i < repo_.GetCount(); ++i) {
    auto repo_obj_res = repo_.GetByIndex(i);
    if (repo_obj_res.has_value() && repo_obj_res.value() == obj) {
      index = i;
      break;
    }
  }

  if (std::cmp_equal(index ,-1)) {
    return std::unexpected(std::runtime_error("DeallocateObject: Object not found in repository"));
  }

  auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

  // Вызов деструктора
  auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
  if (vt_res.has_value()) {
    const VirtualTable* vt = vt_res.value();
    auto dtor_id_res = vt->GetRealFunctionId("_" + vt->GetName() + "_destructor_<M>");
    if (dtor_id_res.has_value()) {
      auto func_res = data.function_repository.GetById(dtor_id_res.value());
      if (func_res.has_value()) {
        // Создать frame, push obj, Execute
        runtime::StackFrame frame;
        frame.function_name = dtor_id_res.value();
        frame.local_variables.emplace_back(obj);
        data.memory.stack_frames.push(frame);
        func_res.value()->Execute(data);
        data.memory.stack_frames.pop();
      }
    }

    // Deallocate
    const size_t size = sizeof(ObjectDescriptor) + vt->GetSize();
    char* raw = reinterpret_cast<char*>(obj);
    allocator_.deallocate(raw, size);
  }

  // Удалить из repo
  auto remove_res = repo_.Remove(index);
  if (!remove_res.has_value()) {
    return std::unexpected(remove_res.error());
  }

  return {};
}

std::expected<void, std::runtime_error> MemoryManager::CollectGarbage(execution_tree::PassedExecutionData& data) {
  if (!gc_.has_value()) {
    return std::unexpected(std::runtime_error("MemoryManager: No GC configured"));
  }
  return gc_.value()->Collect(data);
}

std::expected<void, std::runtime_error> MemoryManager::Clear(execution_tree::PassedExecutionData& data) {
  for (size_t i = 0; i < repo_.GetCount(); ++i) {
    auto obj_res = repo_.GetByIndex(i);
    if (!obj_res.has_value()) {
      continue;
    }
    void* obj = obj_res.value();
    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

    // Получить VT
    auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
    if (!vt_res.has_value()) {
      continue;
    }
    const VirtualTable* vt = vt_res.value();

    // Получить ID деструктора
    auto dtor_id_res = vt->GetRealFunctionId("_" + vt->GetName() + "_destructor_<M>");
    if (dtor_id_res.has_value()) {
      auto func_res = data.function_repository.GetById(dtor_id_res.value());
      if (func_res.has_value()) {
        // Создать frame, push obj, Execute
        runtime::StackFrame frame;
        frame.function_name = dtor_id_res.value();
        frame.local_variables.emplace_back(obj);
        data.memory.stack_frames.push(frame);
        func_res.value()->Execute(data);
        data.memory.stack_frames.pop();
      }
    }

    // Deallocate
    const size_t size = sizeof(ObjectDescriptor) + vt->GetSize();
    char* raw = reinterpret_cast<char*>(obj);
    allocator_.deallocate(raw, size);
  }
  repo_.Clear();
  return {};
}

const ObjectRepository& MemoryManager::GetRepository() const {
  return repo_;
}

} // namespace ovum::vm::runtime
