#include <filesystem>
#include <sstream>

#include <gtest/gtest.h>

#include "lib/vm_ui/vm_ui_functions.hpp"
#include "test_functions.hpp"
#include "test_suites/ProjectIntegrationTestSuite.hpp"

TEST_F(ProjectIntegrationTestSuite, InitTest) {
  ASSERT_TRUE(std::filesystem::is_directory(kTemporaryDirectoryName));
}

TEST_F(ProjectIntegrationTestSuite, NegativeTest1) {
  std::ostringstream out;
  std::istringstream in;
  std::ostringstream err;
  ASSERT_EQ(StartVmConsoleUI(SplitString("test"), out, in, err), 1);
}

TEST_F(ProjectIntegrationTestSuite, NegitiveOutputTest1) {
  std::ostringstream out;
  std::istringstream in;
  std::ostringstream err;
  StartVmConsoleUI(SplitString("test"), out, in, err);
  ASSERT_EQ(err.str(), "Insufficient arguments\n");
}
