#include "vm_ui_functions.hpp"

int32_t StartVmConsoleUI(const std::vector<std::string>& args, std::ostream& out, std::istream& in, std::ostream& err) {
  if (args.size() < 2) {
    err << "Insufficient arguments\n";
    return 1;
  }

  return 0;
}
