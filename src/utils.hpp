#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <string>
#include <stdexcept>

namespace utils {
template<typename T = void*>
using RaiiOwnership = std::shared_ptr<T>;
using RaiiAction = RaiiOwnership<void*>;

template<typename T, typename = std::is_invocable<T>>
auto
make_raii_action(T action) -> RaiiOwnership<void*>
{
  auto custom_deleter = [action](void* ptr) { std::invoke(action); };

  return { nullptr, custom_deleter };
}

template<typename T>
auto
make_raii_deleter(T* ptr, std::function<void(T*)> deleter) -> RaiiOwnership<T>
{
  auto deleter_fn = [deleter](T* ptr) {
    std::invoke(deleter, std::forward<T*>(ptr));
  };
  return std::shared_ptr<T>(std::forward<T*>(ptr), deleter_fn);
}

template<typename T>
T* throw_if_null(T* ptr, const std::string& reason)
{
  if (!ptr) {
    throw std::runtime_error(reason);
  }
  return ptr;
}
} // namespace utils