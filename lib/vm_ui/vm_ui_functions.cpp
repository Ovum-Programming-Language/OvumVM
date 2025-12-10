#include "vm_ui_functions.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <argparser/ArgParser.hpp>

#include "lib/bytecode_lexer/BytecodeLexer.hpp"
#include "lib/bytecode_parser/BytecodeParser.hpp"
#include "lib/bytecode_parser/scenarios/CommandFactory.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/executor/Executor.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/executor/builtin_factory.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

#ifdef JIT_PROVIDED
#include <jit/JitExecutorFactory.hpp>
#endif

constexpr size_t kDefaultJitBoundary = 100000;

std::string ReadFileContent(const std::string& file_path, std::ostream& err) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    err << "Failed to open file: " << file_path << "\n";
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int32_t StartVmConsoleUI(const std::vector<std::string>& args, std::ostream& out, std::istream& in, std::ostream& err) {
  size_t separator_index = args.size();
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "--") {
      separator_index = i;
      break;
    }
  }

  std::vector<std::string> parser_args(args.begin(), args.begin() + static_cast<ptrdiff_t>(separator_index));
  std::vector<std::string> program_args;

  if (separator_index < args.size()) {
    program_args.assign(args.begin() + static_cast<ptrdiff_t>(separator_index + 1), args.end());
  }

  auto is_file = [](std::string& arg) { return std::filesystem::exists(arg); };
  ArgumentParser::ArgParser arg_parser("ovum-vm", PassArgumentTypes());
  arg_parser.AddCompositeArgument('f', "file", "Path to the bytecode file").AddIsGood(is_file).AddValidate(is_file);
  arg_parser.AddUnsignedLongLongArgument('j', "jit-boundary", "JIT compilation boundary").Default(kDefaultJitBoundary);
  arg_parser.AddHelp('h', "help", "Show this help message");

  bool parse_result = arg_parser.Parse(parser_args, {.out_stream = err, .print_messages = true});
  if (!parse_result) {
    err << arg_parser.HelpDescription() << "\n";
    return 1;
  }

  if (arg_parser.Help()) {
    err << arg_parser.HelpDescription() << "\n";
    return 0;
  }

  std::string file_path;
  file_path = arg_parser.GetCompositeValue("file");

  size_t jit_boundary = arg_parser.GetUnsignedLongLongValue("jit-boundary");
  std::string sample = ReadFileContent(file_path, err);

  if (sample.empty()) {
    err << "Failed to read file: " << file_path << "\n";
    return 1;
  }

  ovum::bytecode::lexer::BytecodeLexer lx(sample);
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;
  ovum::vm::runtime::RuntimeMemory memory;
  int32_t return_code = 0;

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
    ovum::bytecode::parser::BytecodeParser bytecode_parser(std::move(jit_factory), jit_boundary, command_factory);

    auto vtable_result = ovum::vm::runtime::RegisterBuiltinVirtualTables(vtable_repo);
    if (!vtable_result) {
      throw std::runtime_error("Failed to register builtin virtual tables: " +
                               std::string(vtable_result.error().what()));
    }

    auto func_result = ovum::vm::execution_tree::RegisterBuiltinFunctions(func_repo);
    if (!func_result) {
      throw std::runtime_error("Failed to register builtin functions: " + std::string(func_result.error().what()));
    }

    const std::vector<ovum::TokenPtr>& tokens = toks.value();
    auto result = bytecode_parser.Parse(tokens, func_repo, vtable_repo, memory);

    if (!result) {
      throw result.error();
    }

    ovum::vm::execution_tree::PassedExecutionData execution_data{
        .memory = memory, .virtual_table_repository = vtable_repo, .function_repository = func_repo};
    ovum::vm::executor::Executor executor(execution_data);
    auto execution_result = executor.RunProgram(result.value(), program_args);

    if (!execution_result) {
      throw std::runtime_error("Execution failed: " + std::string(execution_result.error().what()));
    }

    return_code = static_cast<int32_t>(execution_result.value());
  } catch (const ovum::bytecode::lexer::BytecodeLexerError& e) {
    err << "Lexer error: " << e.what() << "\n";
    return_code = 2;
  } catch (const ovum::bytecode::parser::BytecodeParserError& e) {
    err << "Parser error: " << e.what() << "\n";
    return_code = 3;
  } catch (const std::exception& e) {
    err << "Exception: " << e.what() << "\n";
    return_code = 4;
  }

  std::allocator<char> allocator;

  for (size_t i = 0; i < memory.object_repository.GetCount(); ++i) {
    auto object_result = memory.object_repository.GetByIndex(i);

    if (!object_result.has_value()) {
      err << "Failed to get object: " << object_result.error().what() << "\n";
      return_code = 4;
      continue;
    }

    ovum::vm::runtime::ObjectDescriptor* object = object_result.value();
    uint32_t vtable_index = object->vtable_index;
    auto vtable_result = vtable_repo.GetByIndex(vtable_index);

    if (!vtable_result.has_value()) {
      err << "Failed to get vtable for index " << vtable_index << ": " << vtable_result.error().what() << "\n";
      return_code = 4;
      continue;
    }

    size_t object_size = vtable_result.value()->GetSize();

    allocator.deallocate(reinterpret_cast<char*>(object), object_size);
  }

  return return_code;
}
