#ifndef EXECUTION_TREE_PUREFUNCTION_HPP
#define EXECUTION_TREE_PUREFUNCTION_HPP

#include <algorithm>
#include <cstddef>
#include <expected>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "CacheKey.hpp"
#include "ExecutionConcepts.hpp"
#include "ExecutionResult.hpp"
#include "FunctionRepository.hpp"
#include "IFunctionExecutable.hpp"
#include "PassedExecutionData.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/Variable.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace std {

template<>
struct hash<std::vector<size_t>> {
  static constexpr size_t kHashMultiplier = 0x9e3779b9;
  static constexpr size_t kHashShift = 6;

  size_t operator()(const std::vector<size_t>& vec) const {
    size_t seed = 0;
    for (size_t value : vec) {
      seed ^= std::hash<size_t>{}(value) + kHashMultiplier + (seed << kHashShift) + (seed >> kHashShift);
    }
    return seed;
  }
};

template<>
struct hash<ovum::vm::execution_tree::CacheKey> {
  size_t operator()(const ovum::vm::execution_tree::CacheKey& key) const {
    return std::hash<std::vector<size_t>>{}(key.GetHashValues());
  }
};

} // namespace std

namespace ovum::vm::execution_tree {

template<ExecutableFunction ExecutableFunctionType>
class PureFunction : public IFunctionExecutable {
public:
  PureFunction(const ExecutableFunctionType& function, std::vector<std::string> argument_type_names) :
      function_(function), argument_type_names_(std::move(argument_type_names)) {
    if (argument_type_names_.size() != function_.GetArity()) {
      throw std::runtime_error("PureFunction: argument type names count does not match function arity");
    }
  }

  std::expected<ExecutionResult, std::runtime_error> Execute(PassedExecutionData& execution_data) override {
    const size_t arity = function_.GetArity();

    if (execution_data.memory.machine_stack.size() < arity) {
      return std::unexpected(std::runtime_error("PureFunction: insufficient arguments on stack (expected " +
                                                std::to_string(arity) + ", got " +
                                                std::to_string(execution_data.memory.machine_stack.size()) + ")"));
    }

    // Extract arguments from stack
    std::vector<runtime::Variable> arguments;
    arguments.reserve(arity);

    for (size_t i = 0; i < arity; ++i) {
      arguments.push_back(execution_data.memory.machine_stack.top());
      execution_data.memory.machine_stack.pop();
    }

    std::reverse(arguments.begin(), arguments.end());

    // Verify types and build cache key with hash values
    const auto cache_key_result = CreateCacheKey(arguments, execution_data);

    if (!cache_key_result.has_value()) {
      return std::unexpected(cache_key_result.error());
    }

    const CacheKey& cache_key = cache_key_result.value();

    // Check cache
    auto cache_it = cache_.find(cache_key);

    if (cache_it != cache_.end()) {
      execution_data.memory.machine_stack.push(cache_it->second);
      return ExecutionResult::kNormal;
    }

    // Cache miss - put arguments back on stack and execute
    for (auto& argument : arguments) {
      execution_data.memory.machine_stack.push(argument);
    }

    const std::expected<ExecutionResult, std::runtime_error> result = function_.Execute(execution_data);

    if (!result.has_value() || result.value() != ExecutionResult::kNormal) {
      return result;
    }

    // Get result from stack
    if (execution_data.memory.machine_stack.empty()) {
      return std::unexpected(std::runtime_error("PureFunction: machine stack is empty after execution"));
    }

    const runtime::Variable result_value = execution_data.memory.machine_stack.top();
    execution_data.memory.machine_stack.pop();

    // Cache the result
    cache_[cache_key] = result_value;

    // Put result back on stack
    execution_data.memory.machine_stack.push(result_value);

    return ExecutionResult::kNormal;
  }

  [[nodiscard]] runtime::FunctionId GetId() const override {
    return function_.GetId();
  }

  [[nodiscard]] size_t GetArity() const override {
    return function_.GetArity();
  }

  [[nodiscard]] size_t GetTotalActionCount() const override {
    return function_.GetTotalActionCount();
  }

  [[nodiscard]] size_t GetExecutionCount() const override {
    return function_.GetExecutionCount();
  }

private:
  ExecutableFunctionType function_;
  std::vector<std::string> argument_type_names_;
  std::unordered_map<CacheKey, runtime::Variable> cache_;

  [[nodiscard]] std::expected<size_t, std::runtime_error> GetHash(void* object_ptr,
                                                                  std::string& actual_type,
                                                                  PassedExecutionData& execution_data) const {
    const auto* descriptor = reinterpret_cast<const runtime::ObjectDescriptor*>(object_ptr);
    const auto vtable_result = execution_data.virtual_table_repository.GetByIndex(descriptor->vtable_index);

    if (!vtable_result.has_value()) {
      return std::unexpected(std::runtime_error("PureFunction: failed to get VirtualTable for void* argument"));
    }

    const runtime::VirtualTable* vtable = vtable_result.value();
    actual_type = vtable->GetName();

    std::expected<runtime::FunctionId, std::runtime_error> hash_function_id_result =
        vtable->GetRealFunctionId("_GetHash_<C>");

    if (!hash_function_id_result.has_value()) {
      return std::unexpected(std::runtime_error("PureFunction: failed to get hash function id"));
    }

    const runtime::FunctionId hash_function_id = std::move(hash_function_id_result.value());
    auto hash_function_result = execution_data.function_repository.GetById(hash_function_id);

    if (!hash_function_result.has_value()) {
      return std::unexpected(std::runtime_error("PureFunction: failed to find hash function: " + hash_function_id));
    }

    // Call _GetHash_<C> on a copy of object_ptr
    void* object_ptr_copy = object_ptr;
    execution_data.memory.machine_stack.emplace(object_ptr_copy);
    const auto hash_exec_result = hash_function_result.value()->Execute(execution_data);

    if (!hash_exec_result.has_value() || hash_exec_result.value() != ExecutionResult::kNormal) {
      return std::unexpected(std::runtime_error("PureFunction: hash function execution failed"));
    }

    // Get hash result from stack (should be int64_t)
    if (execution_data.memory.machine_stack.empty()) {
      return std::unexpected(std::runtime_error("PureFunction: machine stack is empty after hash function"));
    }

    const runtime::Variable hash_result = execution_data.memory.machine_stack.top();
    execution_data.memory.machine_stack.pop();

    if (!std::holds_alternative<int64_t>(hash_result)) {
      return std::unexpected(std::runtime_error("PureFunction: hash function did not return int64_t"));
    }

    return static_cast<size_t>(std::get<int64_t>(hash_result));
  }

  [[nodiscard]] std::expected<CacheKey, std::runtime_error> CreateCacheKey(
      const std::vector<runtime::Variable>& arguments, PassedExecutionData& execution_data) const {
    const size_t arity = arguments.size();
    CacheKey cache_key;
    cache_key.GetValues().reserve(arity);
    cache_key.GetHashValues().reserve(arity);

    for (size_t i = 0; i < arity; ++i) {
      const std::string& expected_type = argument_type_names_[i];
      const runtime::Variable& arg = arguments[i];

      std::string actual_type = "undefined";
      size_t value_hash = 0;

      if (std::holds_alternative<int64_t>(arg)) {
        actual_type = "int";
        value_hash = std::hash<int64_t>{}(std::get<int64_t>(arg));
      } else if (std::holds_alternative<double>(arg)) {
        actual_type = "float";
        value_hash = std::hash<double>{}(std::get<double>(arg));
      } else if (std::holds_alternative<bool>(arg)) {
        actual_type = "bool";
        value_hash = std::hash<bool>{}(std::get<bool>(arg));
      } else if (std::holds_alternative<char>(arg)) {
        actual_type = "char";
        value_hash = std::hash<char>{}(std::get<char>(arg));
      } else if (std::holds_alternative<uint8_t>(arg)) {
        actual_type = "byte";
        value_hash = std::hash<uint8_t>{}(std::get<uint8_t>(arg));
      } else if (std::holds_alternative<void*>(arg)) {
        void* object_ptr = std::get<void*>(arg);
        std::expected<size_t, std::runtime_error> hash_result = GetHash(object_ptr, actual_type, execution_data);

        if (!hash_result.has_value()) {
          return std::unexpected(std::runtime_error("PureFunction: failed to get hash for " + actual_type));
        }

        value_hash = hash_result.value();
      } else {
        return std::unexpected(std::runtime_error("PureFunction: unknown variable type"));
      }

      // For void* types, check if the type IS OF the expected type using IsType
      // For primitive types, check exact match
      bool type_matches = false;
      if (std::holds_alternative<void*>(arg)) {
        const auto* descriptor = reinterpret_cast<const runtime::ObjectDescriptor*>(std::get<void*>(arg));
        const auto vtable_result = execution_data.virtual_table_repository.GetByIndex(descriptor->vtable_index);

        if (!vtable_result.has_value()) {
          return std::unexpected(std::runtime_error("PureFunction: failed to get VirtualTable for type checking"));
        }

        type_matches = vtable_result.value()->IsType(expected_type);
      } else {
        type_matches = (actual_type == expected_type);
      }

      if (!type_matches) {
        std::string error_msg = "PureFunction: type mismatch for argument ";
        error_msg += std::to_string(i);
        error_msg += " (expected ";
        error_msg += expected_type;
        error_msg += ", got ";
        error_msg += actual_type;
        error_msg += ")";
        return std::unexpected(std::runtime_error(error_msg));
      }

      cache_key.GetValues().push_back(arg);
      cache_key.GetHashValues().push_back(value_hash);
    }

    return cache_key;
  }
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_PUREFUNCTION_HPP
