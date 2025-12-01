#include "BytecodeParser.hpp"

#include "lib/bytecode_parser/scenarios/CommandParser.hpp"
#include "lib/bytecode_parser/scenarios/FunctionParser.hpp"
#include "lib/bytecode_parser/scenarios/IfParser.hpp"
#include "lib/bytecode_parser/scenarios/InitStaticParser.hpp"
#include "lib/bytecode_parser/scenarios/VtableParser.hpp"
#include "lib/bytecode_parser/scenarios/WhileParser.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"
#include "scenarios/CommandFactory.hpp"

namespace ovum::bytecode::parser {

BytecodeParser::BytecodeParser(std::unique_ptr<vm::executor::IJitExecutorFactory> jit_factory,
                               size_t jit_boundary,
                               std::unique_ptr<ICommandFactory> command_factory) :
    jit_factory_(std::move(jit_factory)), jit_boundary_(jit_boundary) {
  auto cmd_factory = command_factory ? std::move(command_factory) : std::make_unique<CommandFactory>();

  handlers_.push_back(std::make_unique<InitStaticParser>());
  handlers_.push_back(std::make_unique<VtableParser>());
  handlers_.push_back(std::make_unique<FunctionParser>());
  handlers_.push_back(std::make_unique<IfParser>());
  handlers_.push_back(std::make_unique<WhileParser>());
  handlers_.push_back(std::make_unique<CommandParser>(std::move(cmd_factory)));
}

std::expected<std::unique_ptr<vm::execution_tree::Block>, BytecodeParserError> BytecodeParser::Parse(
    const std::vector<TokenPtr>& tokens,
    vm::execution_tree::FunctionRepository& func_repo,
    vm::runtime::VirtualTableRepository& vtable_repo,
    vm::runtime::RuntimeMemory& memory) {
  ParsingSessionData data{.func_repo = func_repo,
                          .vtable_repo = vtable_repo,
                          .memory = memory,
                          .jit_factory = jit_factory_ ? std::optional(std::ref(*jit_factory_)) : std::nullopt,
                          .jit_boundary = jit_boundary_};

  auto session = std::make_shared<ParsingSession>(tokens, data);

  while (!session->IsEof()) {
    bool handled = false;
    for (auto& handler : handlers_) {
      auto result = handler->Handle(session);
      if (result) {
        handled = true;
        break;
      }
      if (result.error().Code() != BytecodeParserErrorCode::kNotMatched) {
        return std::unexpected(result.error());
      }
    }
    if (!handled) {
      const auto* token = session->Current().get();
      std::string message = "Unknown top-level declaration: " + token->GetLexeme() + " at line " +
                            std::to_string(token->GetPosition().GetLine()) + " column " +
                            std::to_string(token->GetPosition().GetColumn());
      return std::unexpected(BytecodeParserError(message));
    }
  }

  return session->ReleaseInitStaticBlock();
}

} // namespace ovum::bytecode::parser
