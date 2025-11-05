#ifndef VM_UI_FUNCTIONS_HPP_
#define VM_UI_FUNCTIONS_HPP_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int32_t StartVmConsoleUI(const std::vector<std::string>& args, std::ostream& out, std::istream& in, std::ostream& err);

#endif // VM_UI_FUNCTIONS_HPP_
