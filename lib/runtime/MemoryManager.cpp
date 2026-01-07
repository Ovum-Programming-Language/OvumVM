#include "MemoryManager.hpp"

#include <utility>

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
  const size_t total_size = sizeof(ObjectDescriptor) + vtable.GetSize();
  char* raw_memory = allocator_.allocate(total_size);
  if (!raw_memory) {
    return std::unexpected(std::runtime_error("MemoryManager: Allocation failed - out of memory"));
  }

  auto* descriptor = new (raw_memory) ObjectDescriptor{};
  descriptor->vtable_index = vtable_index;
  descriptor->badge = 0;

  auto add_result = repo_.Add(descriptor);
  if (!add_result.has_value()) {
    descriptor->~ObjectDescriptor();
    allocator_.deallocate(raw_memory, total_size);
    return std::unexpected(add_result.error());
  }

  descriptor->repo_index = add_result.value();

  if (repo_.GetCount() > gc_threshold_) {
    CollectGarbage(data);
  }

  return reinterpret_cast<void*>(descriptor);
}

std::expected<void, std::runtime_error> MemoryManager::DeallocateObject(void* obj,
                                                                        execution_tree::PassedExecutionData& data) {
  if (!obj) {
    return std::unexpected(std::runtime_error("DeallocateObject: Null object pointer"));
  }

  auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);
  const size_t index = desc->repo_index;

  std::string object_type = "Unknown";
  auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
  if (vt_res.has_value()) {
    object_type = vt_res.value()->GetName();
  }
//  data.error_stream << "DeallocateObject: obj=" << obj
//                    << " repo_index=" << index
//                    << " vtable_index=" << desc->vtable_index
//                    << " type=" << object_type << "\n";

  if (index >= repo_.GetCount()) {
//    data.error_stream << "ERROR: Invalid repo_index " << index << " (repo size=" << repo_.GetCount()
//                      << ") for object of type " << object_type << " at " << obj << "\n";
    return std::unexpected(std::runtime_error("DeallocateObject: Invalid repo_index (out of bounds)"));
  }

  auto check_res = repo_.GetByIndex(index);
  if (!check_res.has_value() || check_res.value() != desc) {
//    data.error_stream << "ERROR: repo_index mismatch for object " << obj
//                      << " (type=" << object_type << "), expected at index " << index << "\n";
    return std::unexpected(std::runtime_error("DeallocateObject: repo_index mismatch"));
  }

  if (vt_res.has_value()) {
    const VirtualTable* vt = vt_res.value();

    auto dtor_id_res = vt->GetRealFunctionId("_destructor_<M>");

    if (dtor_id_res.has_value()) {
      auto func_res = data.function_repository.GetById(dtor_id_res.value());

      if (func_res.has_value()) {
        runtime::StackFrame frame;
        frame.local_variables.emplace_back(obj);
        data.memory.stack_frames.push(std::move(frame));

        auto exec_res = func_res.value()->Execute(data);
        if (!exec_res.has_value()) {
        }

        data.memory.stack_frames.pop();
      }
    }

    const size_t total_size = sizeof(ObjectDescriptor) + vt->GetSize();
    char* raw = reinterpret_cast<char*>(obj);
    desc->~ObjectDescriptor();
    allocator_.deallocate(raw, total_size);
  }

  repo_.Remove(index);

  return {};
}

std::expected<void, std::runtime_error> MemoryManager::Clear(execution_tree::PassedExecutionData& data) {
  while (repo_.GetCount() > 0) {
    const size_t last_index = repo_.GetCount() - 1;
    auto obj_res = repo_.GetByIndex(last_index);

    if (obj_res.has_value()) {
      auto dealloc_res = DeallocateObject(obj_res.value(), data);
      if (!dealloc_res.has_value()) {
//        data.error_stream << "Clear: Deallocate failed for index " << last_index << ": "
//                          << dealloc_res.error().what() << " — forcing Remove\n";
        repo_.Remove(last_index);
      }
    } else {
//      data.error_stream << "Clear: Invalid object at index " << last_index << " — forcing Remove\n";
      repo_.Remove(last_index);
    }
  }

  return {};
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
