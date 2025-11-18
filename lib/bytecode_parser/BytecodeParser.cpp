#include "BytecodeParser.hpp"

#include <tokens/Token.hpp>

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

#include "lib/bytecode_parser/scenarios/FunctionParser.hpp"
#include "scenarios/CommandParser.hpp"
#include "scenarios/IfParser.hpp"
#include "scenarios/InitStaticParser.hpp"
#include "scenarios/VtableParser.hpp"
#include "scenarios/WhileParser.hpp"

namespace ovum::bytecode::parser {

BytecodeParser::BytecodeParser() {
  handlers_.push_back(std::make_unique<InitStaticParser>());
  handlers_.push_back(std::make_unique<VtableParser>());
  handlers_.push_back(std::make_unique<FunctionParser>());
  handlers_.push_back(std::make_unique<IfParser>());
  handlers_.push_back(std::make_unique<WhileParser>());
  handlers_.push_back(std::make_unique<CommandParser>());
}

std::expected<void, BytecodeParserError> BytecodeParser::Parse(const std::vector<TokenPtr>& tokens,
                                                               vm::execution_tree::FunctionRepository& func_repo,
                                                               vm::runtime::VirtualTableRepository& vtable_repo,
                                                               vm::runtime::RuntimeMemory& memory) {
  ParserContext ctx(tokens, func_repo, vtable_repo, memory);

  while (!ctx.IsEof()) {
    bool handled = false;
    for (auto& handler : handlers_) {
      if (handler->Handle(ctx)) {
        handled = true;
        break;
      }
    }
    if (!handled) {
      throw BytecodeParserError("Unknown top-level declaration: " + ctx.Current()->GetLexeme() + " at line " +
                                std::to_string(ctx.Current()->GetPosition().GetLine()) + " column " +
                                std::to_string(ctx.Current()->GetPosition().GetColumn()));
    }
  }

  return {};
}

} // namespace ovum::bytecode::parser
