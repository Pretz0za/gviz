#pragma once

#include <utility>

class IResourceHolder {
public:
  virtual ~IResourceHolder() = default;
};

template <typename T> class ResourceHolder : public IResourceHolder {
public:
  template <typename... Args>
  ResourceHolder(Args &&...args) : m_value(std::forward<Args>(args)...) {}

  T m_value;
};
