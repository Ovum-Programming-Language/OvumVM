#include "GcTestSuite.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/Command.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/PureFunction.hpp"
#include "lib/executor/builtin_factory.hpp"

#include <cassert>

GcTestSuite::GcTestSuite() :
    vtr_(), fr_(), mm_(std::make_unique<ovum::vm::runtime::MarkAndSweepGC>(), kDefaultGCThreshold), rm_() {
}

ovum::vm::execution_tree::PassedExecutionData GcTestSuite::MakeFreshData(uint64_t gc_threshold) {
  mm_ = ovum::vm::runtime::MemoryManager(std::make_unique<ovum::vm::runtime::MarkAndSweepGC>(), gc_threshold);
  ovum::vm::execution_tree::PassedExecutionData data{.memory = rm_,
                                                     .virtual_table_repository = vtr_,
                                                     .function_repository = fr_,
                                                     .memory_manager = mm_,
                                                     .input_stream = std::cin,
                                                     .output_stream = std::cout,
                                                     .error_stream = std::cerr};
  return data;
}

ovum::vm::execution_tree::ExecutionResult NoOpDestructor(ovum::vm::execution_tree::PassedExecutionData& /*data*/) {
  return ovum::vm::execution_tree::ExecutionResult::kNormal;
}

void GcTestSuite::SetUp() {
  vtr_ = ovum::vm::runtime::VirtualTableRepository();

  fr_ = ovum::vm::execution_tree::FunctionRepository();

  vtr_ = ovum::vm::runtime::VirtualTableRepository();

  fr_ = ovum::vm::execution_tree::FunctionRepository();

  {
    auto vt_res = ovum::vm::runtime::RegisterBuiltinVirtualTables(vtr_);
    ASSERT_TRUE(vt_res.has_value()) << "Failed builtin vtables: " << vt_res.error().what();
  }

  {
    auto fn_res = ovum::vm::execution_tree::RegisterBuiltinFunctions(fr_);
    ASSERT_TRUE(fn_res.has_value()) << "Failed builtin functions: " << fn_res.error().what();
  }

  RegisterTestVtables();

  RegisterNoOpDestructors();

  auto gc = std::make_unique<ovum::vm::runtime::MarkAndSweepGC>();
  mm_ = ovum::vm::runtime::MemoryManager(std::move(gc), 10);
}

void GcTestSuite::RegisterTestVtables() {
  // Simple
  {
    ovum::vm::runtime::VirtualTable vt("Simple", sizeof(ovum::vm::runtime::ObjectDescriptor));
    vt.AddFunction("_destructor_<M>", "_Simple_destructor_<M>");
    auto res = vtr_.Add(std::move(vt));
    ASSERT_TRUE(res.has_value()) << "Failed Simple: " << res.error().what();
  }

  // WithRef
  {
    ovum::vm::runtime::VirtualTable vt("WithRef", sizeof(ovum::vm::runtime::ObjectDescriptor) + sizeof(void*));
    vt.AddFunction("_destructor_<M>", "_WithRef_destructor_<M>");
    vt.AddField("Object", sizeof(ovum::vm::runtime::ObjectDescriptor));

    auto res = vtr_.Add(std::move(vt));
    ASSERT_TRUE(res.has_value());
  }

  // Array
  {
    auto scanner = std::make_unique<ovum::vm::runtime::ArrayReferenceScanner>();
    ovum::vm::runtime::VirtualTable vt(
        "Array", sizeof(ovum::vm::runtime::ObjectDescriptor) + sizeof(std::vector<void*>), std::move(scanner));
    vt.AddFunction("_destructor_<M>", "_Array_destructor_<M>");
    auto res = vtr_.Add(std::move(vt));
    ASSERT_TRUE(res.has_value());
  }
}

void GcTestSuite::RegisterNoOpDestructors() {
  const std::vector<std::string> types = {"Simple", "WithRef", "Array"};

  for (const auto& type : types) {
    std::string func_name = "_" + type + "_destructor_<M>";

    // Создаём простое тело — блок с одной командой (no-op)
    auto body = std::make_unique<ovum::vm::execution_tree::Block>();

    auto no_op_cmd = [](ovum::vm::execution_tree::PassedExecutionData& d)
        -> std::expected<ovum::vm::execution_tree::ExecutionResult, std::runtime_error> { return NoOpDestructor(d); };

    body->AddStatement(std::make_unique<ovum::vm::execution_tree::Command<decltype(no_op_cmd)>>(no_op_cmd));

    auto function = std::make_unique<ovum::vm::execution_tree::Function>(func_name, 1u, std::move(body));

    auto res = fr_.Add(std::move(function));

    ASSERT_TRUE(res.has_value()) << "Failed to register destructor for " << type << ": " << res.error().what();
  }
}

std::unordered_set<void*> GcTestSuite::SnapshotRepo(const ovum::vm::runtime::ObjectRepository& repo) const {
  std::unordered_set<void*> snapshot;
  repo.ForAll([&snapshot](void* obj) { snapshot.insert(obj); });
  return snapshot;
}

std::string GcTestSuite::TypeOf(void* obj, const ovum::vm::execution_tree::PassedExecutionData& data) const {
  if (!obj)
    return "<null>";
  auto* desc = reinterpret_cast<ovum::vm::runtime::ObjectDescriptor*>(obj);
  auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
  if (!vt_res.has_value())
    return "<invalid vtable>";
  return vt_res.value()->GetName();
}

bool GcTestSuite::RepoContains(const ovum::vm::runtime::ObjectRepository& repo, void* obj) const {
  bool found = false;
  repo.ForAll([&](void* o) {
    if (o == obj)
      found = true;
  });
  return found;
}

void* GcTestSuite::AllocateTestObject(const std::string& type_name,
                                      ovum::vm::execution_tree::PassedExecutionData& data) {
  auto vt_res = data.virtual_table_repository.GetByName(type_name);
  EXPECT_TRUE(vt_res.has_value()) << "VTable not found: " << type_name;

  auto idx_res = data.virtual_table_repository.GetIndexByName(type_name);
  EXPECT_TRUE(idx_res.has_value());

  auto alloc_res = data.memory_manager.AllocateObject(*vt_res.value(), static_cast<uint32_t>(*idx_res), data);

  EXPECT_TRUE(alloc_res.has_value()) << "Allocation failed for " << type_name;
  return alloc_res.value();
}

void GcTestSuite::SetRef(void* obj, void* target) {
  void** ptr = reinterpret_cast<void**>(reinterpret_cast<char*>(obj) + sizeof(ovum::vm::runtime::ObjectDescriptor));
  *ptr = target;
}

void GcTestSuite::InitArray(void* array_obj) {
  auto* vec = reinterpret_cast<std::vector<void*>*>(reinterpret_cast<char*>(array_obj) +
                                                    sizeof(ovum::vm::runtime::ObjectDescriptor));
  new (vec) std::vector<void*>();
}

void GcTestSuite::AddToArray(void* array_obj, void* item) {
  auto* vec = reinterpret_cast<std::vector<void*>*>(reinterpret_cast<char*>(array_obj) +
                                                    sizeof(ovum::vm::runtime::ObjectDescriptor));
  vec->push_back(item);
}

void GcTestSuite::CollectGarbage(ovum::vm::execution_tree::PassedExecutionData& data) {
  auto res = data.memory_manager.CollectGarbage(data);
  ASSERT_TRUE(res.has_value()) << "GC failed: " << res.error().what();
}
