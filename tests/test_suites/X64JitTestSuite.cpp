#include "X64JitTestSuite.hpp"

#include <filesystem>

#include "lib/vm_ui/vm_ui_functions.hpp"
#include "tests/test_functions.hpp"

void X64JitTestSuite::SetUp() {
  std::filesystem::create_directories(kTemporaryDirectoryName);
}

void X64JitTestSuite::TearDown() {
  std::filesystem::remove_all(kTemporaryDirectoryName);
}

void X64JitTestSuite::RunSingleTest(const JitTestData& test_data) const {
  std::filesystem::path test_file = kTestDataDir;
  test_file /= "examples";
  test_file /= "compiled";
  test_file /= "jit";
  test_file /= test_data.test_name;
  std::string cmd = "ovum-vm -f \"";
  cmd += test_file.string();
  cmd += "\"";
  cmd += " -j ";
  cmd += std::to_string(test_data.jit_action_bound);

  if (!test_data.arguments.empty()) {
    cmd += " -- ";
    cmd += test_data.arguments;
  }

  std::istringstream in(test_data.input);
  std::ostringstream out;
  std::ostringstream err;
  ASSERT_EQ(StartVmConsoleUI(SplitString(cmd), std::cout, in, std::cout), test_data.expected_return_code);
  ASSERT_EQ(out.str(), test_data.expected_output);
  ASSERT_EQ(err.str(), test_data.expected_error);
}
