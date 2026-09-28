#include "catch_amalgamated.hpp"
#include "ecs/exceptions.hpp"

#include <string>

namespace {
struct SomeResource {};
struct SomeComponent {};
} // namespace

TEST_CASE("MissingResourceException is a std::runtime_error mentioning the "
         "resource type",
         "[ecs][exceptions]") {
  try {
    throw MissingResourceException<SomeResource>();
  } catch (const std::runtime_error &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("not found in ECS") != std::string::npos);
  }
}

TEST_CASE("UinitializedResourceException reports the resource as present but "
         "uninitialized",
         "[ecs][exceptions]") {
  try {
    throw UinitializedResourceException<SomeResource>();
  } catch (const std::runtime_error &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("unintialized") != std::string::npos);
  }
}

TEST_CASE("MissingComponentException includes the offending entity id",
         "[ecs][exceptions]") {
  try {
    throw MissingComponentException<SomeComponent>(EntityID(123));
  } catch (const std::runtime_error &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("123") != std::string::npos);
  }
}
