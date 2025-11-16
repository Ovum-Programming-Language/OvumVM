#include "lib/bytecode_lexer/BytecodeLexer.hpp"
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
  } catch (const std::exception& e) {
    err << "Lexer error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
