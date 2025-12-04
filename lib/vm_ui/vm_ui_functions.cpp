#include "lib/bytecode_lexer/BytecodeLexer.hpp"
#include "lib/bytecode_parser/BytecodeParser.hpp"
#include "lib/bytecode_parser/scenarios/PlaceholderCommandFactory.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/IFunctionExecutable.hpp"
#include "lib/executor/PlaceholderJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"
#include "vm_ui_functions.hpp"

int32_t StartVmConsoleUI(const std::vector<std::string>& args, std::ostream& out, std::istream& in, std::ostream& err) {
  if (args.size() < 2) {
    err << "Insufficient arguments\n";
    return 1;
  }

  const std::string& sample = args[1];
  ovum::bytecode::lexer::BytecodeLexer lx(sample);

  try {
    auto toks = lx.Tokenize();

    if (!toks) {
      throw toks.error();
    }

    for (auto& t : toks.value()) {
      out << t->ToString() << "\n";
    }

    out << "\n\n\n";

    auto factory = std::make_unique<ovum::vm::executor::PlaceholderJitExecutorFactory>();

    ovum::bytecode::parser::PlaceholderCommandFactory command_factory =
        ovum::bytecode::parser::PlaceholderCommandFactory();

    ovum::bytecode::parser::BytecodeParser parser(std::move(factory), 1, command_factory);
    ovum::vm::execution_tree::FunctionRepository func_repo;
    ovum::vm::runtime::VirtualTableRepository vtable_repo;
    ovum::vm::runtime::RuntimeMemory memory;

    auto tokens = toks.value();

    auto result = parser.Parse(tokens, func_repo, vtable_repo, memory);

    if (!result) {
      throw result.error();
    }

  } catch (const ovum::bytecode::lexer::BytecodeLexerError& e) {
    err << "Lexer error: " << e.what() << "\n";
    return 1;
  } catch (const ovum::bytecode::parser::BytecodeParserError& e) {
    err << "Parser error: " << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    err << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
