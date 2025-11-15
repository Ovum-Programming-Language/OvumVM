#include "NewlineHandler.hpp"

#include "tokens/TokenFactory.hpp"

namespace ovum::bytecode::lexer {

OptToken NewlineHandler::Scan(BytecodeSourceWrapper& wrapper) {
  return std::make_optional(ovum::TokenFactory::MakeNewline(wrapper.GetLine() - 1, wrapper.GetTokenCol()));
}

} // namespace ovum::bytecode::lexer
