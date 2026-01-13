
#include <gtest/gtest.h>

#include "lib/vm_ui/vm_ui_functions.hpp"
#include "test_functions.hpp"
#include "test_suites/X64JitTestSuite.hpp"

TEST_F(X64JitTestSuite, Test1) {
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