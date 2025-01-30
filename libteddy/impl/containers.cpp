#include <libteddy/impl/containers.hpp>

#include <cstdlib>
#include <cstring>

namespace teddy::details {

template<class T>
array<T>::array(const int32 size) :
  length_(size),
  data_(static_cast<T *>(std::calloc(as_usize(length_), sizeof(T))))
{
}

template<class T>
array<T>::array(const array<T> &other) :
  length_(other.length_)
{
  std::memcpy(data_, other.data_, as_usize(length_) * sizeof(T));
}

template<class T>
array<T>::array(array<T> &&other) noexcept :
  length_(tools::exchange(other.length_, 0)),
  data_(tools::exchange(other.data_, nullptr))
{
}

template<class T>
array<T>::~array() {
  std::free(data_);
}

template<class T>
auto array<T>::operator=(const array<T> &other) -> array<T> & {
  if (this != &other) {
    T *old_data = data_;
    T *new_data = static_cast<T *>(
      std::malloc(as_usize(other.length_) * sizeof(T)));
    if (new_data != nullptr) {
      std::memcpy(new_data, other.data_, as_usize(other.length_) * sizeof(T));
      data_ = new_data;
      length_ = other.length_;
      std::free(old_data);
    }
  }
  return *this;
}

template<class T>
auto array<T>::operator=(array<T> &&other) noexcept -> array<T> & {
  if (this != &other) {
    length_ = tools::exchange(other.length_, 0);
    data_ = tools::exchange(other.data_, nullptr);
  }
  return *this;
}

template<class T>
auto array<T>::operator[](int32 i) -> T & {
  return data_[i];
}

template<class T>
auto array<T>::operator[](int32 i) const -> const T & {
  return data_[i];
}

template<class T>
auto array<T>::size() const noexcept -> int32 {
  return length_;
}

template class array<int32>;

} // namespace teddy::details
