#include "lib/bytecode_lexer/BytecodeLexer.hpp"
#include "lib/bytecode_parser/BytecodeParser.hpp"
#include "lib/bytecode_parser/scenarios/CommandFactory.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/executor/Executor.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/executor/builtin_factory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"
#include "vm_ui_functions.hpp"

#ifdef JIT_PROVIDED
#include <jit/JitExecutorFactory.hpp>
#endif

constexpr size_t kJitBoundary = 100000;

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

#ifdef JIT_PROVIDED
    std::unique_ptr<ovum::vm::executor::IJitExecutorFactory> jit_factory =
        std::make_unique<ovum::vm::jit::JitExecutorFactory>();
#else
    std::unique_ptr<ovum::vm::executor::IJitExecutorFactory> jit_factory = nullptr;
#endif

    ovum::bytecode::parser::CommandFactory command_factory = ovum::bytecode::parser::CommandFactory();
    ovum::bytecode::parser::BytecodeParser parser(std::move(jit_factory), kJitBoundary, command_factory);
    ovum::vm::execution_tree::FunctionRepository func_repo;
    ovum::vm::runtime::VirtualTableRepository vtable_repo;
    ovum::vm::runtime::RuntimeMemory memory;

    auto vtable_result = ovum::vm::runtime::RegisterBuiltinVirtualTables(vtable_repo);
    if (!vtable_result) {
      throw std::runtime_error("Failed to register builtin virtual tables: " +
                               std::string(vtable_result.error().what()));
    }

    auto func_result = ovum::vm::execution_tree::RegisterBuiltinFunctions(func_repo);
    if (!func_result) {
      throw std::runtime_error("Failed to register builtin functions: " + std::string(func_result.error().what()));
    }

    auto tokens = toks.value();
    auto result = parser.Parse(tokens, func_repo, vtable_repo, memory);

    if (!result) {
      throw result.error();
    }

    ovum::vm::execution_tree::PassedExecutionData execution_data{
        .memory = memory, .virtual_table_repository = vtable_repo, .function_repository = func_repo};
    ovum::vm::executor::Executor executor(execution_data);
    auto execution_result = executor.RunProgram(result.value());

    if (!execution_result) {
      throw std::runtime_error("Execution failed: " + std::string(execution_result.error().what()));
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
