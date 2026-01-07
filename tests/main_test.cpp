#include <chrono>
#include <filesystem>
#include <sstream>

#include <gtest/gtest.h>

#include "lib/vm_ui/vm_ui_functions.hpp"
#include "test_functions.hpp"
#include "test_suites/ProjectIntegrationTestSuite.hpp"

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
  ASSERT_EQ(
      err.str(),
      "Not enough values were passed to argument --file.\n"
      "ovum-vm\nShow this help message\n\nOPTIONS:\n"
      "-f,  --file=<CompositeString>:  Path to the bytecode file\n"
      "-j,  --jit-boundary=<unsigned long long>:  JIT compilation boundary [default = 100000]\n"
      "-m,  --max-objects=<unsigned long long>:  Maximum number of objects to keep in memory [default = 10000]\n\n"
      "-h,  --help:  Display this help and exit\n\n");
}

TEST_F(ProjectIntegrationTestSuite, HelpTest) {
  std::ostringstream out;
  std::istringstream in;
  std::ostringstream err;
  StartVmConsoleUI(SplitString("test --help"), out, in, err);
  ASSERT_EQ(
      err.str(),
      "ovum-vm\nShow this help message\n\nOPTIONS:\n"
      "-f,  --file=<CompositeString>:  Path to the bytecode file\n"
      "-j,  --jit-boundary=<unsigned long long>:  JIT compilation boundary [default = 100000]\n"
      "-m,  --max-objects=<unsigned long long>:  Maximum number of objects to keep in memory [default = 10000]\n\n"
      "-h,  --help:  Display this help and exit\n\n");
}

TEST_F(ProjectIntegrationTestSuite, FibTest1) {
  RunSingleTest(TestData{
      .test_name = "fib.oil",
      .arguments = "3",
      .input = "",
      .expected_output = "2\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, FibTest2) {
  RunSingleTest(TestData{
      .test_name = "fib.oil",
      .arguments = "92",
      .input = "",
      .expected_output = "7540113804746346429\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, FactTest1) {
  RunSingleTest(TestData{
      .test_name = "fact.oil",
      .arguments = "6",
      .input = "",
      .expected_output = "720\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, FactTest2) {
  RunSingleTest(TestData{
      .test_name = "fact.oil",
      .arguments = "18",
      .input = "",
      .expected_output = "6402373705728000\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, FactTest3) {
  RunSingleTest(TestData{
      .test_name = "fact.oil",
      .arguments = "21",
      .input = "",
      .expected_output = "-4249290049419214848\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, ShapesTest1) {
  RunSingleTest(TestData{
      .test_name = "shapes.oil",
      .arguments = "",
      .input = "3 5\n5\n",
      .expected_output = "Enter width and height: \n"
                         "Enter radius: \n"
                         "Area: 15.000000, Perimeter: 16.000000\n"
                         "Area: 78.539750, Perimeter: 31.415900\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, ShapesTest2) {
  RunSingleTest(TestData{
      .test_name = "shapes.oil",
      .arguments = "",
      .input = "2.5 0.1\n0.2\n",
      .expected_output = "Enter width and height: \n"
                         "Enter radius: \n"
                         "Area: 0.250000, Perimeter: 5.200000\n"
                         "Area: 0.125664, Perimeter: 1.256636\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, PrimesTest1) {
  RunSingleTest(TestData{
      .test_name = "primes.oil",
      .arguments = "",
      .input = "10\n",
      .expected_output = "Enter the maximum number to find primes: Prime numbers up to 10:\n"
                         "2\n3\n5\n7\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, PrimesTest2) {
  RunSingleTest(TestData{
      .test_name = "primes.oil",
      .arguments = "",
      .input = "100\n",
      .expected_output =
          "Enter the maximum number to find primes: Prime numbers up to 100:\n"
          "2\n3\n5\n7\n11\n13\n17\n19\n23\n29\n31\n37\n41\n43\n47\n53\n59\n61\n67\n71\n73\n79\n83\n89\n97\n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, SortTest1) {
  RunSingleTest(TestData{
      .test_name = "sort.oil",
      .arguments = "",
      .input = "5\n5 3 1 4 2\n",
      .expected_output = "1 2 3 4 5 \n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, SortTest2) {
  RunSingleTest(TestData{
      .test_name = "sort.oil",
      .arguments = "",
      .input = "10\n1 1 1 1 1 1 -10 1 2 0\n",
      .expected_output = "-10 0 1 1 1 1 1 1 1 2 \n",
      .expected_error = "",
      .expected_return_code = 0,
  });
}

TEST_F(ProjectIntegrationTestSuite, InteropTest1) {
  RunSingleTest(TestData{
      .test_name = "checker.oil",
      .arguments = "",
      .input = "1\n",
      .expected_output = "Enter the ID to check: 1\n"
                         "ID is incorrect\n",
      .expected_error = "",
      .expected_return_code = 1,
  });
}

TEST_F(ProjectIntegrationTestSuite, InteropTest2) {
  RunSingleTest(TestData{
      .test_name = "checker.oil",
      .arguments = "",
      .input = "OVUM-059BD7BB64BF-I6BDCCSV-19\n",
      .expected_output = "Enter the ID to check: OVUM-059BD7BB64BF-I6BDCCSV-19\n"
                         "ID is incorrect\n",
      .expected_error = "",
      .expected_return_code = 1,
  });
}

TEST_F(ProjectIntegrationTestSuite, InteropTest3) {
  constexpr int64_t kAdded = 239ll;
  constexpr int64_t kDivisor = 997ll;
  constexpr auto kBinaryFilter = static_cast<int64_t>(0xA12BC3456DE89F70ull);
#ifdef WITH_VALGRIND
  constexpr int64_t kMaxAllowedError = 1000000000ll; // I assume that 1 s is enough for the difference between
                                                     // end of test and call to time from FFI library with Valgrind
#else
  constexpr int64_t kMaxAllowedError = 10000000ll; // I assume that 10 ms is enough for the difference between
                                                   // end of test and call to time from FFI library
#endif

  TestData test_data{
      .test_name = "checker.oil",
      .arguments = "",
      .input = "OVUM-55BC4C33548D-RTXPNG6D-24\n",
      .expected_output =
          "Enter the ID to check: OVUM-55BC4C33548D-RTXPNG6D-24\n"
          "ID is correct, your code: ",
      .expected_error = "",
      .expected_return_code = 0,
  };

  std::filesystem::path test_file = kTestDataDir;
  test_file /= "examples";
  test_file /= "compiled";
  test_file /= test_data.test_name;
  std::string cmd = "ovum-vm -f \"";
  cmd += test_file.string();
  cmd += "\"";

  if (!test_data.arguments.empty()) {
    cmd += " -- ";
    cmd += test_data.arguments;
  }

  std::istringstream in(test_data.input);
  std::ostringstream out;
  std::ostringstream err;
  ASSERT_EQ(StartVmConsoleUI(SplitString(cmd), out, in, err), test_data.expected_return_code);
  int64_t current_nanotime =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  ASSERT_EQ(err.str(), test_data.expected_error);
  ASSERT_TRUE(out.str().starts_with(test_data.expected_output));
  ASSERT_TRUE(out.str().ends_with("\n"));
  std::string code_string =
      out.str().substr(test_data.expected_output.size(), out.str().size() - test_data.expected_output.size() - 1);
  int64_t code = std::stoll(code_string);
  int64_t time_from_code = code - kAdded;
  time_from_code *= kDivisor;
  time_from_code ^= kBinaryFilter;
  ASSERT_LE(std::abs(time_from_code - current_nanotime), kMaxAllowedError);
}
