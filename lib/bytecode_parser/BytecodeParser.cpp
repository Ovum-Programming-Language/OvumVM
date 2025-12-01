#include "BytecodeParser.hpp"

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

#include "lib/bytecode_parser/scenarios/CommandParser.hpp"
#include "lib/bytecode_parser/scenarios/FunctionParser.hpp"
#include "lib/bytecode_parser/scenarios/IfParser.hpp"
#include "lib/bytecode_parser/scenarios/InitStaticParser.hpp"
#include "lib/bytecode_parser/scenarios/VtableParser.hpp"
#include "lib/bytecode_parser/scenarios/WhileParser.hpp"

namespace ovum::bytecode::parser {

BytecodeParser::BytecodeParser(std::unique_ptr<vm::executor::IJitExecutorFactory> jit_factory, size_t jit_boundary) :
    jit_factory_(std::move(jit_factory)), jit_boundary_(jit_boundary) {
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
  auto ctx = std::make_shared<ParserContext>(tokens, func_repo, vtable_repo, memory, jit_factory_.get(), jit_boundary_);

  while (!ctx->IsEof()) {
    bool handled = false;

    for (auto& handler : handlers_) {
      auto result = handler->Handle(ctx);

      if (result) {
        handled = true;
        break;
      }

      if (result.error().Code() != BytecodeParserErrorCode::kNotMatched) {
        return std::unexpected(result.error());
      }
    }

    if (!handled) {
      const auto* token = ctx->Current().get();
      std::string message = "Unknown top-level declaration: " + token->GetLexeme() + " at line " +
                            std::to_string(token->GetPosition().GetLine()) + " column " +
                            std::to_string(token->GetPosition().GetColumn());

      throw BytecodeParserError(message);
    }
  }

  return {};
}

} // namespace ovum::bytecode::parser
