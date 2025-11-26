#ifndef RUNTIME_VIRTUALTABLEREPOSITORY_HPP
#define RUNTIME_VIRTUALTABLEREPOSITORY_HPP

#include <cstddef>
#include <expected>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "VirtualTable.hpp"

namespace ovum::vm::runtime {

class VirtualTableRepository {
public:
  VirtualTableRepository();

  void Reserve(size_t count);

  [[nodiscard]] std::expected<size_t, std::runtime_error> Add(VirtualTable table);

  [[nodiscard]] std::expected<VirtualTable*, std::runtime_error> GetByIndex(size_t index);

  [[nodiscard]] std::expected<const VirtualTable*, std::runtime_error> GetByIndex(size_t index) const;

  [[nodiscard]] std::expected<VirtualTable*, std::runtime_error> GetByName(const std::string& name);

  [[nodiscard]] std::expected<const VirtualTable*, std::runtime_error> GetByName(const std::string& name) const;

  [[nodiscard]] std::expected<const size_t, std::runtime_error> GetIndexByName(const std::string& name) const;

  [[nodiscard]] size_t GetCount() const;

private:
  std::vector<VirtualTable> vtables_;
  std::unordered_map<std::string, size_t> index_by_name_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_VIRTUALTABLEREPOSITORY_HPP
