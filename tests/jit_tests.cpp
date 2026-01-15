
#include <gtest/gtest.h>

#include "test_suites/X64JitTestSuite.hpp"

TEST_F(X64JitTestSuite, Test1JitEnable) {
  RunSingleTest(JitTestData{
      .test_name = "jit-float-test.oil",
      .arguments = "",
      .input = "",
      .expected_output = "",
      .expected_error = "",
      .expected_return_code = 0,
      .jit_action_bound = 1,
  });
}

TEST_F(X64JitTestSuite, Test2JitDisable) {
  constexpr size_t kJitActionBound = 10000000;
  RunSingleTest(JitTestData{
      .test_name = "jit-float-test.oil",
      .arguments = "",
      .input = "",
      .expected_output = "",
      .expected_error = "",
      .expected_return_code = 0,
      .jit_action_bound = kJitActionBound,
  });
}
