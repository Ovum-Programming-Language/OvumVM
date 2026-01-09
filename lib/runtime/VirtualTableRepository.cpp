#include "VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

VirtualTableRepository::VirtualTableRepository() = default;

void VirtualTableRepository::Reserve(size_t count) {
  vtables_.reserve(count);
}

std::expected<size_t, std::runtime_error> VirtualTableRepository::Add(VirtualTable table) {
  const std::string name = table.GetName();

  if (index_by_name_.find(name) != index_by_name_.end()) {
    return std::unexpected(std::runtime_error("VirtualTable with the same name already exists: " + name));
  }

  vtables_.emplace_back(std::move(table));
  index_by_name_[name] = vtables_.size() - 1U;

  return vtables_.size() - 1U;
}

std::expected<VirtualTable*, std::runtime_error> VirtualTableRepository::GetByIndex(size_t index) {
  if (index >= vtables_.size()) {
    return std::unexpected(std::runtime_error("VirtualTable index out of range"));
  }

  return &vtables_[index];
}

std::expected<const VirtualTable*, std::runtime_error> VirtualTableRepository::GetByIndex(size_t index) const {
  if (index >= vtables_.size()) {
    return std::unexpected(std::runtime_error("VirtualTable index out of range"));
  }

  return &vtables_[index];
}

std::expected<VirtualTable*, std::runtime_error> VirtualTableRepository::GetByName(const std::string& name) {
  const auto it = index_by_name_.find(name);

  if (it == index_by_name_.end()) {
    return std::unexpected(std::runtime_error("VirtualTable not found by name: " + name));
  }

  return &vtables_[it->second];
}

std::expected<const VirtualTable*, std::runtime_error> VirtualTableRepository::GetByName(
    const std::string& name) const {
  const auto it = index_by_name_.find(name);

  if (it == index_by_name_.end()) {
    return std::unexpected(std::runtime_error("VirtualTable not found by name: " + name));
  }

  return &vtables_[it->second];
}

std::expected<size_t, std::runtime_error> VirtualTableRepository::GetIndexByName(const std::string& name) const {
  const auto it = index_by_name_.find(name);

  if (it == index_by_name_.end()) {
    return std::unexpected(std::runtime_error("VirtualTable not found by name: " + name));
  }

  return it->second;
}

size_t VirtualTableRepository::GetCount() const {
  return vtables_.size();
}

} // namespace ovum::vm::runtime
