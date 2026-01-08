#include "MarkAndSweepGC.hpp"

#include <queue>
#include <stack>
#include <vector>

#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

std::expected<void, std::runtime_error> MarkAndSweepGC::Collect(execution_tree::PassedExecutionData& data) {
  Mark(data);
  Sweep(data);
  return {};
}

void MarkAndSweepGC::Mark(execution_tree::PassedExecutionData& data) {
  std::queue<void*> worklist;
  AddRoots(worklist, data);

  while (!worklist.empty()) {
    void* obj = worklist.front();
    worklist.pop();

    if (obj == nullptr) {
      continue;
    }

    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);
    if (desc->badge & kMarkBit) {
      continue;
    }
    desc->badge |= kMarkBit;

    auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
    if (!vt_res.has_value()) {
      continue;
    }
    const VirtualTable* vt = vt_res.value();

    vt->ScanReferences(obj, [&](void* ref) {
      if (ref) {
        worklist.push(ref);
      }
    });
  }
}

void MarkAndSweepGC::Sweep(execution_tree::PassedExecutionData& data) {
  std::vector<void*> to_delete;
  to_delete.reserve(data.memory_manager.GetRepository().GetCount() / 4);

  const ObjectRepository& repo = data.memory_manager.GetRepository();
  repo.ForAll([&to_delete](void* obj) {
    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

    if (!(desc->badge & kMarkBit)) {
      to_delete.push_back(obj);
    }

    desc->badge &= ~kMarkBit;
  });

  data.error_stream << "[GC Sweep] Starting sweep. Total objects in repo: "
                    << repo.GetCount()
                    << ", objects to delete: " << to_delete.size() << "\n";

  for (auto obj : to_delete) {
    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

    auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
    std::string type_name = "<unknown type>";
    if (vt_res.has_value()) {
      type_name = vt_res.value()->GetName();
    }

    data.error_stream << "[GC Sweep] Deleting object: "
                      << type_name
                      << " at address " << obj << "\n";
  }

  for (auto obj : to_delete) {
    auto dealloc_res = data.memory_manager.DeallocateObject(obj, data);
    if (!dealloc_res) {
      data.error_stream << dealloc_res.error().what();
    }
  }
}

void MarkAndSweepGC::AddRoots(std::queue<void*>& worklist, execution_tree::PassedExecutionData& data) {
  data.error_stream << "[GC Roots] Scanning " << data.memory.stack_frames.size() << " frames\n";
  size_t root_count = 0;

  for (const auto& var : data.memory.global_variables) {
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);
      if (ptr) {
        worklist.push(ptr);
      }

      if (ptr) {
        root_count++;
        auto* desc = reinterpret_cast<ObjectDescriptor*>(ptr);
        auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
        std::string name = vt_res ? vt_res.value()->GetName() : "unknown";
        data.error_stream << "[GC Root] Found live object: " << name << " at " << ptr << "\n";
      }
      data.error_stream << "[GC Roots] Total roots: " << root_count << "\n";
    }
  }


  std::stack<StackFrame> temp_stack_frames;
  std::swap(temp_stack_frames, data.memory.stack_frames);

  while (!temp_stack_frames.empty()) {
    const StackFrame& frame = temp_stack_frames.top();
    for (const auto& var : frame.local_variables) {
      if (std::holds_alternative<void*>(var)) {
        void* ptr = std::get<void*>(var);
        if (ptr) {
          worklist.push(ptr);
          ++root_count;
          auto* desc = reinterpret_cast<ObjectDescriptor*>(ptr);
          auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
          std::string name = vt_res ? vt_res.value()->GetName() : "unknown";
          data.error_stream << "[GC Root] Found live object: " << name << " at " << ptr << "\n";
        }
      }
    }
    temp_stack_frames.pop();
  }

  std::swap(temp_stack_frames, data.memory.stack_frames);


  std::stack<Variable> temp_machine_stack;
  std::swap(temp_machine_stack, data.memory.machine_stack);

  while (!temp_machine_stack.empty()) {
    const Variable& var = temp_machine_stack.top();
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);
      if (ptr) {
        worklist.push(ptr);
        ++root_count;
        auto* desc = reinterpret_cast<ObjectDescriptor*>(ptr);
        auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
        std::string name = vt_res ? vt_res.value()->GetName() : "unknown";
        data.error_stream << "[GC Root] Found live object: " << name << " at " << ptr << "\n";
      }
    }
    temp_machine_stack.pop();
  }

  std::swap(temp_machine_stack, data.memory.machine_stack);
}

} // namespace ovum::vm::runtime
