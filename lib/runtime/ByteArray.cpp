#include "ByteArray.hpp"

#include <cstring>
#include <stdexcept>

namespace ovum::vm::runtime {

std::allocator<char> ByteArray::allocator_;

ByteArray::ByteArray() = default;

ByteArray::ByteArray(size_t size) : size_(size), capacity_(size) {
  if (size > 0) {
    AllocateMemory(size);
    std::memset(data_, 0, size);
  }
}

ByteArray::ByteArray(void* data, size_t capacity) :
    data_(static_cast<uint8_t*>(data)), size_(capacity), capacity_(capacity), is_view_(true) {
}

ByteArray::ByteArray(const ByteArray& other) : size_(other.size_), capacity_(other.capacity_) {
  if (other.capacity_ > 0) {
    AllocateMemory(other.capacity_);
    std::memcpy(data_, other.data_, other.capacity_);
  }
}

ByteArray::ByteArray(ByteArray&& other) noexcept :
    data_(other.data_), size_(other.size_), capacity_(other.capacity_), is_view_(other.is_view_) {
  other.data_ = nullptr;
  other.size_ = 0;
  other.capacity_ = 0;
  other.is_view_ = false;
}

ByteArray& ByteArray::operator=(const ByteArray& other) {
  if (this == &other) {
    return *this;
  }

  if (other.size_ > capacity_ || is_view_) {
    DeallocateMemory();
    AllocateMemory(other.capacity_);
  }

  size_ = other.size_;
  capacity_ = other.capacity_;
  is_view_ = false;

  if (other.size_ > 0) {
    std::memcpy(data_, other.data_, other.size_);
  }

  return *this;
}

ByteArray& ByteArray::operator=(ByteArray&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  DeallocateMemory();

  data_ = other.data_;
  size_ = other.size_;
  capacity_ = other.capacity_;
  is_view_ = other.is_view_;

  other.data_ = nullptr;
  other.size_ = 0;
  other.capacity_ = 0;
  other.is_view_ = false;

  return *this;
}

ByteArray::~ByteArray() {
  DeallocateMemory();
}

void ByteArray::Insert(size_t index, uint8_t value) {
  if (index > size_) {
    throw std::runtime_error("ByteArray: Insert index out of bounds");
  }

  CheckReallocationAllowed();

  if (size_ >= capacity_) {
    Reserve(capacity_ == 0 ? 1 : capacity_ * 2);
  }

  std::memmove(data_ + index + 1, data_ + index, size_ - index);
  data_[index] = value;
  ++size_;
}

void ByteArray::Insert(size_t index, const uint8_t* data, size_t count) {
  if (index > size_) {
    throw std::runtime_error("ByteArray: Insert index out of bounds");
  }
  if (data == nullptr && count > 0) {
    throw std::runtime_error("ByteArray: Insert data is null");
  }

  CheckReallocationAllowed();

  while (size_ + count > capacity_) {
    Reserve(capacity_ == 0 ? 1 : capacity_ * 2);
  }

  std::memmove(data_ + index + count, data_ + index, size_ - index);
  std::memcpy(data_ + index, data, count);
  size_ += count;
}

void ByteArray::Remove(size_t index) {
  Remove(index, 1);
}

void ByteArray::Remove(size_t index, size_t count) {
  if (index >= size_) {
    throw std::runtime_error("ByteArray: Remove index out of bounds");
  }
  if (index + count > size_) {
    throw std::runtime_error("ByteArray: Remove count exceeds available elements");
  }

  CheckReallocationAllowed();

  std::memmove(data_ + index, data_ + index + count, size_ - index - count);
  size_ -= count;
}

bool ByteArray::operator==(const ByteArray& other) const {
  if (size_ != other.size_) {
    return false;
  }
  if (size_ == 0) {
    return true;
  }
  return std::memcmp(data_, other.data_, size_) == 0;
}

bool ByteArray::operator<(const ByteArray& other) const {
  if (size_ < other.size_) {
    return true;
  }
  if (size_ > other.size_) {
    return false;
  }
  if (size_ == 0) {
    return false;
  }
  return std::memcmp(data_, other.data_, size_) < 0;
}

void ByteArray::Resize(size_t new_size) {
  CheckReallocationAllowed();

  if (new_size > capacity_) {
    Reserve(new_size);
  }

  if (new_size > size_) {
    std::memset(data_ + size_, 0, new_size - size_);
  }

  size_ = new_size;
}

void ByteArray::Reserve(size_t new_capacity) {
  CheckReallocationAllowed();

  if (new_capacity <= capacity_) {
    return;
  }

  char* new_data = allocator_.allocate(new_capacity);
  auto* new_data_uint8 = reinterpret_cast<uint8_t*>(new_data);
  if (size_ > 0) {
    std::memcpy(new_data_uint8, data_, size_);
  }

  DeallocateMemory();
  data_ = new_data_uint8;
  capacity_ = new_capacity;
}

void ByteArray::ShrinkToFit() {
  CheckReallocationAllowed();

  if (size_ == capacity_) {
    return;
  }

  if (size_ == 0) {
    DeallocateMemory();
    capacity_ = 0;
    return;
  }

  char* new_data = allocator_.allocate(size_);
  auto* new_data_uint8 = reinterpret_cast<uint8_t*>(new_data);
  std::memcpy(new_data_uint8, data_, size_);

  DeallocateMemory();
  data_ = new_data_uint8;
  capacity_ = size_;
}

void ByteArray::Clear() {
  CheckReallocationAllowed();
  size_ = 0;
}

size_t ByteArray::GetHash() const {
  if (size_ == 0) {
    return 0;
  }

  // Simple hash function (FNV-1a variant)
  size_t hash = kHashOffsetBasis;
  for (size_t i = 0; i < size_; ++i) {
    hash ^= static_cast<size_t>(data_[i]);
    hash *= kHashFnvPrime;
  }
  return hash;
}

size_t ByteArray::Size() const {
  return size_;
}

size_t ByteArray::Capacity() const {
  return capacity_;
}

uint8_t& ByteArray::operator[](size_t index) {
  if (index >= size_) {
    throw std::runtime_error("ByteArray: Index out of bounds");
  }
  return data_[index];
}

const uint8_t& ByteArray::operator[](size_t index) const {
  if (index >= size_) {
    throw std::runtime_error("ByteArray: Index out of bounds");
  }
  return data_[index];
}

uint8_t* ByteArray::Data() {
  return data_;
}

const uint8_t* ByteArray::Data() const {
  return data_;
}

bool ByteArray::IsView() const {
  return is_view_;
}

void ByteArray::CheckReallocationAllowed() const {
  if (is_view_) {
    throw std::runtime_error("ByteArray: Cannot reallocate memory for a view");
  }
}

void ByteArray::AllocateMemory(size_t capacity) {
  if (capacity > 0) {
    char* allocated = allocator_.allocate(capacity);
    data_ = reinterpret_cast<uint8_t*>(allocated);
  } else {
    data_ = nullptr;
  }
}

void ByteArray::DeallocateMemory() {
  if (is_view_) {
    return;
  }

  if (data_ != nullptr) {
    char* data_char = reinterpret_cast<char*>(data_);
    allocator_.deallocate(data_char, capacity_);
    data_ = nullptr;
  }
}

} // namespace ovum::vm::runtime
