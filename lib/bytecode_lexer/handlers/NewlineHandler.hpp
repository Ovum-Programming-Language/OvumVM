#ifndef NEWLINEHANDLER_HPP_
#define NEWLINEHANDLER_HPP_

#include "Handler.hpp"

namespace ovum::bytecode::lexer {

class NewlineHandler final : public Handler {
public:
  OptToken Scan(BytecodeSourceWrapper& wrapper) override;
};

} // namespace ovum::bytecode::lexer

#endif // NEWLINEHANDLER_HPP_
