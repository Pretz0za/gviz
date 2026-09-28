// Handle<Tag> is a strongly-typed, phantom-tagged wrapper around EntityID.

#include "catch_amalgamated.hpp"
#include "ecs/handle.hpp"

#include <type_traits>
#include <unordered_map>

namespace {
struct TagA {};
struct TagB {};
using HandleA = Handle<TagA>;
} // namespace

TEST_CASE("A default-constructed Handle is invalid", "[ecs][handle]") {
  HandleA h;
  REQUIRE_FALSE(h.IsValid());
  REQUIRE(h.Raw() == HandleA::kInvalid);
}

TEST_CASE("An explicitly constructed Handle carries its raw id and is valid",
         "[ecs][handle]") {
  HandleA h(42);
  REQUIRE(h.IsValid());
  REQUIRE(h.Raw() == 42u);
}

TEST_CASE("Handles compare equal iff their raw ids match", "[ecs][handle]") {
  HandleA a(5);
  HandleA b(5);
  HandleA c(6);

  REQUIRE(a == b);
  REQUIRE_FALSE(a == c);
  REQUIRE(a != c);
  REQUIRE_FALSE(a != b);
}

TEST_CASE("A Handle constructed with kInvalid reports itself as invalid",
         "[ecs][handle]") {
  HandleA h(HandleA::kInvalid);
  REQUIRE_FALSE(h.IsValid());
}

TEST_CASE("Handle is hashable and usable as an unordered_map key",
         "[ecs][handle]") {
  std::unordered_map<HandleA, int> m;
  m[HandleA(1)] = 100;
  m[HandleA(2)] = 200;

  REQUIRE(m.at(HandleA(1)) == 100);
  REQUIRE(m.at(HandleA(2)) == 200);
  REQUIRE(m.find(HandleA(3)) == m.end());
}

TEST_CASE("Handle<TagA> and Handle<TagB> are distinct types", "[ecs][handle]") {
  // Compile-time property: different tags produce different Handle
  // instantiations, preventing accidental mixing of e.g. NodeID and EdgeID.
  STATIC_REQUIRE_FALSE(std::is_same_v<Handle<TagA>, Handle<TagB>>);
}
