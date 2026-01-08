#include "MemoryManager.hpp"

#include <utility>
#include <vector>

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"

#include "ObjectDescriptor.hpp"

namespace ovum::vm::runtime {

MemoryManager::MemoryManager(std::unique_ptr<IGarbageCollector> gc, size_t max_objects) :
    gc_(std::move(gc)), gc_threshold_(max_objects) {
}

std::expected<void*, std::runtime_error> MemoryManager::AllocateObject(const VirtualTable& vtable,
                                                                       uint32_t vtable_index,
                                                                       execution_tree::PassedExecutionData& data) {
  if (repo_.GetCount() > gc_threshold_) {
    std::expected<void, std::runtime_error> collect_res = CollectGarbage(data);

    if (!collect_res.has_value()) {
      return std::unexpected(collect_res.error());
    }
  }

  const size_t total_size = vtable.GetSize();
  char* raw_memory = nullptr;

  try {
    raw_memory = allocator_.allocate(total_size);
  } catch (const std::bad_alloc&) {
    return std::unexpected(std::runtime_error("MemoryManager: Allocation failed - out of memory"));
  }

  auto* descriptor = reinterpret_cast<ObjectDescriptor*>(raw_memory);
  descriptor->vtable_index = vtable_index;
  descriptor->badge = 0;

  std::memset(raw_memory + sizeof(ObjectDescriptor), 0, total_size - sizeof(ObjectDescriptor));

  std::expected<void, std::runtime_error> add_result = repo_.Add(descriptor);

  if (!add_result.has_value()) {
    allocator_.deallocate(raw_memory, total_size);
    return std::unexpected(add_result.error());
  }

  return reinterpret_cast<void*>(descriptor);
}

std::expected<void, std::runtime_error> MemoryManager::DeallocateObject(void* obj,
                                                                        execution_tree::PassedExecutionData& data) {
  if (!obj) {
    return std::unexpected(std::runtime_error("DeallocateObject: Null object pointer"));
  }

  auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

  std::expected<const VirtualTable*, std::runtime_error> vt_res =
      data.virtual_table_repository.GetByIndex(desc->vtable_index);

  if (!vt_res.has_value()) {
    return std::unexpected(std::runtime_error("DeallocateObject: Virtual table not found for index " +
                                              std::to_string(desc->vtable_index)));
  }

  std::string object_type = vt_res.value()->GetName();
  const VirtualTable* vt = vt_res.value();

  std::expected<FunctionId, std::runtime_error> dtor_id_res = vt->GetRealFunctionId("_destructor_<M>");

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
  std::expected<execution_tree::ExecutionResult, std::runtime_error> exec_res = func_res.value()->Execute(data);
  data.memory.stack_frames.pop();

  if (!exec_res.has_value()) {
    return std::unexpected(exec_res.error());
  }

  const size_t total_size = vt->GetSize();
  char* raw = reinterpret_cast<char*>(obj);

  std::expected<void, std::runtime_error> remove_res = repo_.Remove(desc);

  if (!remove_res.has_value()) {
    return std::unexpected(remove_res.error());
  }

  allocator_.deallocate(raw, total_size);

  return {};
}

std::expected<void, std::runtime_error> MemoryManager::Clear(execution_tree::PassedExecutionData& data) {
  std::vector<void*> objects_to_clear;
  objects_to_clear.reserve(repo_.GetCount());

  repo_.ForAll([&objects_to_clear](void* obj) { objects_to_clear.push_back(obj); });

  std::optional<std::runtime_error> first_error;

  for (void* obj : objects_to_clear) {
    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

    std::expected<const VirtualTable*, std::runtime_error> vt_res =
        data.virtual_table_repository.GetByIndex(desc->vtable_index);

    if (!vt_res.has_value()) {
      if (!first_error) {
        first_error =
            std::runtime_error("Clear: Virtual table not found for index " + std::to_string(desc->vtable_index));
      }
      continue;
    }

    const VirtualTable* vt = vt_res.value();

    std::expected<FunctionId, std::runtime_error> dtor_id_res = vt->GetRealFunctionId("_destructor_<M>");

    if (dtor_id_res.has_value()) {
      auto func_res = data.function_repository.GetById(dtor_id_res.value());

      if (func_res.has_value()) {
        runtime::StackFrame frame = {.function_name = "Object deallocation (Clear)"};
        data.memory.machine_stack.emplace(obj);
        data.memory.stack_frames.push(std::move(frame));
        std::expected<execution_tree::ExecutionResult, std::runtime_error> exec_res = func_res.value()->Execute(data);
        data.memory.stack_frames.pop();

        if (!exec_res.has_value()) {
          if (!first_error) {
            first_error = exec_res.error();
          }
          continue;
        }
      }
    }

    std::expected<void, std::runtime_error> remove_res = repo_.Remove(desc);

    if (!remove_res.has_value()) {
      if (!first_error) {
        first_error = remove_res.error();
      }
      continue;
    }

    const size_t total_size = vt->GetSize();
    char* raw = reinterpret_cast<char*>(obj);

    allocator_.deallocate(raw, total_size);
  }

  repo_.Clear();

  if (first_error.has_value()) {
    return std::unexpected(*first_error);
  }

  return {};
}

std::expected<void, std::runtime_error> MemoryManager::CollectGarbage(execution_tree::PassedExecutionData& data) {
  if (!gc_) {
    return std::unexpected(std::runtime_error("MemoryManager: No GC configured"));
  }

  return gc_->Collect(data);
}

const ObjectRepository& MemoryManager::GetRepository() const {
  return repo_;
}

} // namespace ovum::vm::runtime
