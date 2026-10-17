#pragma once

#include <cstdint>
#include <cstring>
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

  // Cheap fingerprint over the raw field values, used by non-real-time
  // layout algorithms to detect that a config was edited since the last
  // Tick() without re-running the layout itself.
  uint64_t Hash() {
    uint64_t h = 1469598103934665603ull;
    for (const ConfigField &field : Fields()) {
      uint64_t bits = 0;
      switch (field.type) {
      case ConfigFieldType::Double:
        std::memcpy(&bits, field.value, sizeof(double));
        break;
      case ConfigFieldType::Float: {
        uint32_t v;
        std::memcpy(&v, field.value, sizeof(float));
        bits = v;
        break;
      }
      case ConfigFieldType::Int32:
      case ConfigFieldType::UInt32: {
        uint32_t v;
        std::memcpy(&v, field.value, sizeof(uint32_t));
        bits = v;
        break;
      }
      case ConfigFieldType::UInt8: {
        uint8_t v;
        std::memcpy(&v, field.value, sizeof(uint8_t));
        bits = v;
        break;
      }
      case ConfigFieldType::Bool: {
        bool v;
        std::memcpy(&v, field.value, sizeof(bool));
        bits = v ? 1 : 0;
        break;
      }
      }
      h ^= bits;
      h *= 1099511628211ull;
    }
    return h;
  }
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
