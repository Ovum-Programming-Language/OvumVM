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
  ASSERT_EQ(err.str(),
            "Not enough values were passed to argument --file.\n"
            "ovum-vm\nShow this help message\n\nOPTIONS:\n"
            "-f,  --file=<CompositeString>:  Path to the bytecode file\n"
            "-j,  --jit-boundary=<unsigned long long>:  JIT compilation boundary [default = 100000]\n\n"
            "-h,  --help:  Display this help and exit\n\n");
}

TEST_F(ProjectIntegrationTestSuite, HelpTest) {
  std::ostringstream out;
  std::istringstream in;
  std::ostringstream err;
  StartVmConsoleUI(SplitString("test --help"), out, in, err);
  ASSERT_EQ(err.str(),
            "ovum-vm\nShow this help message\n\nOPTIONS:\n"
            "-f,  --file=<CompositeString>:  Path to the bytecode file\n"
            "-j,  --jit-boundary=<unsigned long long>:  JIT compilation boundary [default = 100000]\n\n"
            "-h,  --help:  Display this help and exit\n\n");
}
