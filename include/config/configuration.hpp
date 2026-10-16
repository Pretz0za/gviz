#pragma once

#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

enum class ConfigFieldType { Double, Float, Int32, UInt32, UInt8, Bool };

struct ConfigField {
  std::string name;
  void *value;
  ConfigFieldType type;
  double min = 0.0;
  double max = 0.0;
};

class IConfiguration {
public:
  virtual ~IConfiguration() = default;
  virtual std::vector<ConfigField> Fields() = 0;
};

template <typename T>
class MissingConfigurationException : public std::runtime_error {
public:
  explicit MissingConfigurationException()
      : std::runtime_error(std::string("Configuration ") + typeid(T).name() +
                           " was not supplied") {};
};

// Validates in a member-initializer list, so a configurable that hands
// sub-configurations to members it owns (&m_config->childConfig) throws
// instead of dereferencing null.
template <typename C> C *RequireConfiguration(C *config) {
  if (config == nullptr)
    throw MissingConfigurationException<C>();
  return config;
}
