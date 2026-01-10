#include <gtest/gtest.h>

#include "tests/test_suites/GcTestSuite.hpp"

#include <memory>
#include <stdexcept>
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

TEST_F(GcTestSuite, UnreachableObjectCollected) {
  auto data = MakeFreshData();

  void* obj = AllocateTestObject("Simple", data);

  auto before = SnapshotRepo(mm_.GetRepository());
  ASSERT_EQ(before.size(), 1u);
  ASSERT_TRUE(RepoContains(mm_.GetRepository(), obj));

  CollectGarbage(data);

  auto after = SnapshotRepo(mm_.GetRepository());
  EXPECT_EQ(after.size(), 0u);
  EXPECT_FALSE(RepoContains(mm_.GetRepository(), obj));
}

TEST_F(GcTestSuite, RootInGlobalVariablesSurvives) {
  auto data = MakeFreshData();

  void* obj = AllocateTestObject("Simple", data);
  data.memory.global_variables.emplace_back(obj);

  CollectGarbage(data);

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), obj));
  EXPECT_EQ(SnapshotRepo(mm_.GetRepository()).size(), 1u);
}

TEST_F(GcTestSuite, RootInMachineStackSurvives) {
  auto data = MakeFreshData();

  void* obj = AllocateTestObject("Simple", data);
  data.memory.machine_stack.emplace(obj);

  CollectGarbage(data);

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), obj));
}

TEST_F(GcTestSuite, TransitiveReachabilityThroughField) {
  auto data = MakeFreshData();

  void* root = AllocateTestObject("WithRef", data);
  void* child = AllocateTestObject("Simple", data);

  SetRef(root, child);

  data.memory.global_variables.emplace_back(root);

  CollectGarbage(data);

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), root));
  EXPECT_TRUE(RepoContains(mm_.GetRepository(), child));
  EXPECT_EQ(SnapshotRepo(mm_.GetRepository()).size(), 2u);
}

TEST_F(GcTestSuite, TransitiveReachabilityThroughArray) {
  auto data = MakeFreshData();

  void* arr = AllocateTestObject("Array", data);
  InitArray(arr);

  void* child1 = AllocateTestObject("Simple", data);
  void* child2 = AllocateTestObject("Simple", data);

  AddToArray(arr, child1);
  AddToArray(arr, child2);

  data.memory.global_variables.emplace_back(arr);

  CollectGarbage(data);

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), arr));
  EXPECT_TRUE(RepoContains(mm_.GetRepository(), child1));
  EXPECT_TRUE(RepoContains(mm_.GetRepository(), child2));
  EXPECT_EQ(SnapshotRepo(mm_.GetRepository()).size(), 3u);
}

TEST_F(GcTestSuite, CycleWithoutRootsCollected) {
  auto data = MakeFreshData();

  void* objA = AllocateTestObject("WithRef", data);
  void* objB = AllocateTestObject("WithRef", data);

  SetRef(objA, objB);
  SetRef(objB, objA);

  CollectGarbage(data);

  EXPECT_FALSE(RepoContains(mm_.GetRepository(), objA));
  EXPECT_FALSE(RepoContains(mm_.GetRepository(), objB));
  EXPECT_EQ(SnapshotRepo(mm_.GetRepository()).size(), 0u);
}

TEST_F(GcTestSuite, CycleWithRootPreserved) {
  auto data = MakeFreshData();

  void* objA = AllocateTestObject("WithRef", data);
  void* objB = AllocateTestObject("WithRef", data);

  SetRef(objA, objB);
  SetRef(objB, objA);

  data.memory.global_variables.emplace_back(objA);

  CollectGarbage(data);

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), objA));
  EXPECT_TRUE(RepoContains(mm_.GetRepository(), objB));
  EXPECT_EQ(SnapshotRepo(mm_.GetRepository()).size(), 2u);
}

TEST_F(GcTestSuite, MultipleRootsDifferentPlaces) {
  auto data = MakeFreshData();

  void* global = AllocateTestObject("Simple", data);
  void* stack = AllocateTestObject("Simple", data);
  void* local = AllocateTestObject("Simple", data);

  data.memory.global_variables.emplace_back(global);
  data.memory.machine_stack.emplace(stack);

  ovum::vm::runtime::StackFrame frame;
  frame.local_variables.emplace_back(local);
  data.memory.stack_frames.push(std::move(frame));

  CollectGarbage(data);

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), global));
  EXPECT_TRUE(RepoContains(mm_.GetRepository(), stack));
  EXPECT_TRUE(RepoContains(mm_.GetRepository(), local));
  EXPECT_EQ(SnapshotRepo(mm_.GetRepository()).size(), 3u);
}

TEST_F(GcTestSuite, SmallThresholdFrequentAllocations) {
  auto data = MakeFreshData(3);

  std::vector<void*> objects;
  std::vector<void*> should_survive;

  const int k_iterations = 15;

  for (int i = 0; i < k_iterations; ++i) {
    void* obj = AllocateTestObject("Simple", data);
    objects.push_back(obj);

    if (i % 3 == 0) {
      data.memory.global_variables.emplace_back(obj);
      should_survive.push_back(obj);
    }

    auto gc_res = data.memory_manager.CollectGarbageIfRequired(data);
    ASSERT_TRUE(gc_res.has_value());
  }

  CollectGarbage(data);

  auto snapshot = SnapshotRepo(mm_.GetRepository());
  EXPECT_GE(snapshot.size(), should_survive.size());

  for (void* obj : should_survive) {
    EXPECT_TRUE(RepoContains(mm_.GetRepository(), obj));
  }
}

TEST_F(GcTestSuite, NullReferencesNotCrashing) {
  auto data = MakeFreshData();

  void* root = AllocateTestObject("WithRef", data);
  SetRef(root, nullptr);

  data.memory.global_variables.emplace_back(root);

  ASSERT_NO_FATAL_FAILURE(CollectGarbage(data));

  EXPECT_TRUE(RepoContains(mm_.GetRepository(), root));
}
