#ifndef GC_TEST_SUITE_HPP
#define GC_TEST_SUITE_HPP

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/MemoryManager.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/ObjectRepository.hpp"
#include "lib/runtime/VirtualTable.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"
#include "lib/runtime/gc/MarkAndSweepGC.hpp"
#include "lib/runtime/gc/reference_scanners/ArrayReferenceScanner.hpp"
#include "lib/runtime/gc/reference_scanners/DefaultReferenceScanner.hpp"

const uint64_t kDefaultGCThreshold = 100;

ovum::vm::execution_tree::ExecutionResult NoOpDestructor(ovum::vm::execution_tree::PassedExecutionData& data);

class GcTestSuite : public ::testing::Test {
protected:
  GcTestSuite();

  void SetUp() override;
  void TearDown() override;

  ovum::vm::execution_tree::PassedExecutionData MakeFreshData(uint64_t gc_threshold = kDefaultGCThreshold);

  void RegisterTestVtables();
  void RegisterNoOpDestructors();

  [[nodiscard]] std::unordered_set<void*> SnapshotRepo(const ovum::vm::runtime::ObjectRepository& repo) const;

  std::string TypeOf(void* obj, const ovum::vm::execution_tree::PassedExecutionData& data) const;

  bool RepoContains(const ovum::vm::runtime::ObjectRepository& repo, void* obj) const;

  void* AllocateTestObject(const std::string& type_name, ovum::vm::execution_tree::PassedExecutionData& data);

  void SetRef(void* obj, void* target);

  void InitArray(void* array_obj);

  void AddToArray(void* array_obj, void* item);

  void CollectGarbage(ovum::vm::execution_tree::PassedExecutionData& data);

protected:
  ovum::vm::runtime::VirtualTableRepository vtr_;
  ovum::vm::execution_tree::FunctionRepository fr_;
  ovum::vm::runtime::MemoryManager mm_;
  ovum::vm::runtime::RuntimeMemory rm_;
};

#endif // GC_TEST_SUITE_HPP
