#ifndef PROJECTINTEGRATIONTESTSUITE_HPP_
#define PROJECTINTEGRATIONTESTSUITE_HPP_

#include <string>

#include <gtest/gtest.h>

struct TestData {
  std::string test_name;
  std::string arguments;
  std::string input;
  std::string expected_output;
  std::string expected_error;
  int32_t expected_return_code = 0;
};

struct ProjectIntegrationTestSuite : public testing::Test { // special test structure
  const std::string kTemporaryDirectoryName = "./gtest_tmp";
  const std::string kTestDataDir = TEST_DATA_DIR;

  void SetUp() override; // method that is called at the beginning of every test

  void TearDown() override; // method that is called at the end of every test

  void RunSingleTest(const TestData& test_data) const;
};

#endif // PROJECTINTEGRATIONTESTSUITE_HPP_
