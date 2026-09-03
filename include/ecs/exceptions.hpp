#include "ecs/icomponent_pool.hpp"
#include <stdexcept>
#include <string>

template <typename T> class MissingResourceException : public std::runtime_error {
public:
  explicit MissingResourceException()
      : std::runtime_error(std::string("Resource ") + typeid(T).name() + " not found in ECS") {};
};

template <typename T>
class MissingComponentException : public std::runtime_error {
public:
  explicit MissingComponentException(const EntityID &id)
      : std::runtime_error("Expected entity " + std::to_string(id) +
                           " to have component " + typeid(T).name()) {}
};
