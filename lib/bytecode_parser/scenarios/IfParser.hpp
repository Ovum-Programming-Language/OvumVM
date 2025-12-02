#ifndef BYTECODE_PARSER_IFPARSER_HPP_
#define BYTECODE_PARSER_IFPARSER_HPP_

#include <expected>
#include <memory>

#include "ICommandFactory.hpp"
#include "IParserHandler.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParsingSession.hpp"

namespace ovum::bytecode::parser {

class IfParser : public IParserHandler {
public:
  explicit IfParser(const ICommandFactory& factory);
  std::expected<void, BytecodeParserError> Handle(std::shared_ptr<ParsingSession> ctx) override;

private:
  const ICommandFactory& factory_;
};

} // namespace ovum::bytecode::parser

#endif // BYTECODE_PARSER_IFPARSER_HPP_
