#include "BytecodeParser.hpp"

#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

#include "scenarios/CommandFactory.hpp"
#include "scenarios/CommandParser.hpp"
#include "scenarios/FunctionParser.hpp"
#include "scenarios/IfParser.hpp"
#include "scenarios/InitStaticParser.hpp"
#include "scenarios/VtableParser.hpp"
#include "scenarios/WhileParser.hpp"

namespace ovum::bytecode::parser {

BytecodeParser::BytecodeParser(std::unique_ptr<vm::executor::IJitExecutorFactory> jit_factory,
                               size_t jit_boundary,
                               const ICommandFactory& command_factory) :
    jit_factory_(std::move(jit_factory)), jit_boundary_(jit_boundary) {
  handlers_.push_back(std::make_unique<InitStaticParser>(command_factory));
  handlers_.push_back(std::make_unique<VtableParser>());
  handlers_.push_back(std::make_unique<FunctionParser>(command_factory));
  handlers_.push_back(std::make_unique<IfParser>(command_factory));
  handlers_.push_back(std::make_unique<WhileParser>(command_factory));
  handlers_.push_back(std::make_unique<CommandParser>(command_factory));
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

  std::shared_ptr<ParsingSession> session = std::make_shared<ParsingSession>(tokens, data);

  while (!session->IsEof()) {
    bool handled = false;

    for (std::unique_ptr<IParserHandler>& handler : handlers_) {
      std::expected<void, BytecodeParserError> result = handler->Handle(session);

      if (result) {
        handled = true;
        break;
      }

      if (result.error().Code() != BytecodeParserErrorCode::kNotMatched) {
        return std::unexpected(result.error());
      }
    }

    if (!handled) {
      TokenPtr token = session->Current();
      std::string message = "Unknown top-level declaration: " + token->GetLexeme() + " at line " +
                            std::to_string(token->GetPosition().GetLine()) + " column " +
                            std::to_string(token->GetPosition().GetColumn());

      return std::unexpected(BytecodeParserError(message));
    }
  }

  return session->ReleaseInitStaticBlock();
}

} // namespace ovum::bytecode::parser
