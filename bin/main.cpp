#include <iostream>
#include <string>
#include <vector>

#include "lib/vm_ui/vm_ui_functions.hpp"

int main(int32_t argc, char** argv) {
  std::vector<std::string> args = std::vector<std::string>(argv, argv + argc);

  return StartVmConsoleUI(args, std::cout, std::cin, std::cerr);
}
