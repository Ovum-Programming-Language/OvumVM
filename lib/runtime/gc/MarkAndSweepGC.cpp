#include "MarkAndSweepGC.hpp"

#include <queue>
#include <ranges>
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

    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);
    if (desc->badge & kMarkBit) {
      continue;  // Already marked
    }
    desc->badge |= kMarkBit;

    auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
    if (!vt_res.has_value()) {
      // Error handling: skip for now
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

  const ObjectRepository& repo = data.memory_manager.GetRepository();
  for (size_t i = 0; i < repo.GetCount(); ++i) {
    auto obj_res = repo.GetByIndex(i);
    if (!obj_res.has_value()) {
      continue;
    }

    const ObjectDescriptor* const_desc = obj_res.value();

    void* obj = const_cast<void*>(static_cast<const void*>(const_desc));

    if (!(const_desc->badge & kMarkBit)) {
      to_delete.push_back(obj);
    }

    // Сброс mark-bit для всех объектов
    const_cast<ObjectDescriptor*>(const_desc)->badge &= ~kMarkBit;
  }

  for (void* obj : to_delete) {
    auto dealloc_res = data.memory_manager.DeallocateObject(obj, data);
    if (!dealloc_res.has_value()) {
      // Log error, continue
    }
  }
}

void MarkAndSweepGC::AddRoots(std::queue<void*>& worklist, execution_tree::PassedExecutionData& data) {
  // Scan global variables
  for (const auto& var : data.memory.global_variables) {
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);
      if (ptr) {
        worklist.push(ptr);
      }
    }
  }

  // Scan stack frames
  std::vector<StackFrame> temp_frames;
  while (!data.memory.stack_frames.empty()) {
    temp_frames.push_back(data.memory.stack_frames.top());
    data.memory.stack_frames.pop();
  }
  for (const auto& frame : temp_frames) {
    for (const auto& var : frame.local_variables) {
      if (std::holds_alternative<void*>(var)) {
        void* ptr = std::get<void*>(var);
        if (ptr) {
          worklist.push(ptr);
        }
      }
    }
  }
  for (auto & temp_frame : std::ranges::reverse_view(temp_frames)) {
    data.memory.stack_frames.push(temp_frame);
  }

  // Scan machine stack
  std::vector<Variable> temp_stack;
  while (!data.memory.machine_stack.empty()) {
    temp_stack.push_back(data.memory.machine_stack.top());
    data.memory.machine_stack.pop();
  }
  for (const auto& var : temp_stack) {
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);
      if (ptr) {
        worklist.push(ptr);
      }
    }
  }
  for (auto & it : std::ranges::reverse_view(temp_stack)) {
    data.memory.machine_stack.push(it);
  }
}

} // namespace ovum::vm::runtime
