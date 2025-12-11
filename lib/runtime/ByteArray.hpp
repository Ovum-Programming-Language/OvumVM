#ifndef RUNTIME_BYTEARRAY_HPP
#define RUNTIME_BYTEARRAY_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>

namespace ovum::vm::runtime {

class ByteArray {
public:
  // Default constructor - creates empty array
  ByteArray();

  // Constructor with initial size
  explicit ByteArray(size_t size);

  // Constructor from void* pointer and capacity (creates a view)
  ByteArray(void* data, size_t capacity);

  // Copy constructor (rule of five)
  ByteArray(const ByteArray& other);

  // Move constructor (rule of five)
  ByteArray(ByteArray&& other) noexcept;

  // Copy assignment operator (rule of five)
  ByteArray& operator=(const ByteArray& other);

  // Move assignment operator (rule of five)
  ByteArray& operator=(ByteArray&& other) noexcept;

  // Destructor (rule of five)
  ~ByteArray();

  // Insertion at index
  void Insert(size_t index, uint8_t value);
  void Insert(size_t index, const uint8_t* data, size_t count);

  // Removal at index
  void Remove(size_t index);
  void Remove(size_t index, size_t count);

  // Comparison operators
  [[nodiscard]] bool operator==(const ByteArray& other) const;
  [[nodiscard]] bool operator<(const ByteArray& other) const;

  // Resizing
  void Resize(size_t new_size);

  // Reserving capacity
  void Reserve(size_t new_capacity);

  // Shrinking to fit
  void ShrinkToFit();

  // Clearing
  void Clear();

  // Getting hash
  [[nodiscard]] size_t GetHash() const;

  // Size and capacity
  [[nodiscard]] size_t Size() const;
  [[nodiscard]] size_t Capacity() const;

  // Access operators
  [[nodiscard]] uint8_t& operator[](size_t index);
  [[nodiscard]] const uint8_t& operator[](size_t index) const;

  // Data access
  [[nodiscard]] uint8_t* Data();
  [[nodiscard]] const uint8_t* Data() const;

  // Check if it's a view
  [[nodiscard]] bool IsView() const;

private:
  uint8_t* data_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 0;
  bool is_view_ = false;

  static constexpr size_t kHashOffsetBasis = 2166136261u;
  static constexpr size_t kHashFnvPrime = 16777619u;

  static std::allocator<char> allocator_;

  void CheckReallocationAllowed() const;
  void AllocateMemory(size_t capacity);
  void DeallocateMemory();
};

} // namespace ovum::vm::runtime

// Hash function for ByteArray (for use in unordered containers)
namespace std {
template<>
struct hash<ovum::vm::runtime::ByteArray> {
  size_t operator()(const ovum::vm::runtime::ByteArray& arr) const {
    return arr.GetHash();
  }
};
} // namespace std

#endif // RUNTIME_BYTEARRAY_HPP
