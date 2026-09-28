#include "catch_amalgamated.hpp"
#include "ecs/resource.hpp"

#include <memory>
#include <string>

TEST_CASE("ResourceHolder forwards constructor arguments into m_value",
         "[ecs][resource]") {
  ResourceHolder<int> holder(42);
  REQUIRE(holder.m_value == 42);
}

TEST_CASE("ResourceHolder supports multi-argument construction",
         "[ecs][resource]") {
  ResourceHolder<std::string> holder(size_t{3}, 'x');
  REQUIRE(holder.m_value == "xxx");
}

TEST_CASE("ResourceHolder<T> is usable through the IResourceHolder base "
         "pointer, as Graph/Subgraph store it",
         "[ecs][resource]") {
  std::unique_ptr<IResourceHolder> base =
      std::make_unique<ResourceHolder<int>>(7);
  auto *typed = static_cast<ResourceHolder<int> *>(base.get());
  REQUIRE(typed->m_value == 7);
}
