#ifndef BYTECODELEXERTESTSUITE_HPP_
#define BYTECODELEXERTESTSUITE_HPP_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "lib/bytecode_lexer/BytecodeLexer.hpp"

struct BytecodeLexerTestSuite : public testing::Test {
  void SetUp() override;
  void TearDown() override;

  // Helper methods to create lexer instances
  ovum::bytecode::lexer::BytecodeLexer CreateLexer(std::string_view src);

  // Helper methods to verify tokenization results
  void AssertTokenizationSuccess(const std::string& input, size_t expected_token_count);
  void AssertTokenizationError(const std::string& input, const std::string& expected_error_substring);

  // Helper methods to verify tokens
  void AssertTokenCount(const std::vector<ovum::TokenPtr>& tokens, size_t expected_count);
  void AssertTokenExists(const std::vector<ovum::TokenPtr>& tokens, size_t index);
  void AssertTokenIsEof(const std::vector<ovum::TokenPtr>& tokens,
                        size_t index,
                        int32_t expected_line,
                        int32_t expected_col);
  void AssertTokenIsNewline(const std::vector<ovum::TokenPtr>& tokens,
                            size_t index,
                            int32_t expected_line,
                            int32_t expected_col);
  void AssertTokenIsIdentifier(const std::vector<ovum::TokenPtr>& tokens,
                               size_t index,
                               const std::string& expected_value,
                               int32_t expected_line = -1,
                               int32_t expected_col = -1);
  void AssertTokenIsKeyword(const std::vector<ovum::TokenPtr>& tokens,
                            size_t index,
                            const std::string& expected_value,
                            int32_t expected_line = -1,
                            int32_t expected_col = -1);
  void AssertTokenIsIntLiteral(const std::vector<ovum::TokenPtr>& tokens,
                               size_t index,
                               const std::string& expected_lexeme,
                               int64_t expected_value,
                               int32_t expected_line = -1,
                               int32_t expected_col = -1);
  void AssertTokenIsFloatLiteral(const std::vector<ovum::TokenPtr>& tokens,
                                 size_t index,
                                 const std::string& expected_lexeme,
                                 double expected_value,
                                 int32_t expected_line = -1,
                                 int32_t expected_col = -1);
  void AssertTokenIsStringLiteral(const std::vector<ovum::TokenPtr>& tokens,
                                  size_t index,
                                  const std::string& expected_raw,
                                  const std::string& expected_value,
                                  int32_t expected_line = -1,
                                  int32_t expected_col = -1);
  void AssertTokenIsPunct(const std::vector<ovum::TokenPtr>& tokens,
                          size_t index,
                          const std::string& expected_value,
                          int32_t expected_line = -1,
                          int32_t expected_col = -1);

  // Helper to tokenize and return tokens (for detailed inspection)
  std::vector<ovum::TokenPtr> TokenizeSuccessfully(const std::string& input);
};

#endif // BYTECODELEXERTESTSUITE_HPP_
