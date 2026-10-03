#include "catch_amalgamated.hpp"
#include "ds/state_machine.hpp"
#include <algorithm>

enum class Light { Red, Yellow, Green };

TEST_CASE("StateMachine starts in its initial state", "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  REQUIRE(sm.Current() == Light::Red);
}

TEST_CASE("AddTransition allows a matching Transition to succeed",
         "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  sm.AddTransition(Light::Red, Light::Green);

  REQUIRE(sm.CanTransition(Light::Green));
  REQUIRE(sm.Transition(Light::Green));
  REQUIRE(sm.Current() == Light::Green);
}

TEST_CASE("Transition fails when no edge allows it", "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  sm.AddTransition(Light::Red, Light::Green);

  REQUIRE_FALSE(sm.CanTransition(Light::Yellow));
  REQUIRE_FALSE(sm.Transition(Light::Yellow));
  REQUIRE(sm.Current() == Light::Red);
}

TEST_CASE("Transitions are directed", "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  sm.AddTransition(Light::Red, Light::Green);

  REQUIRE(sm.Transition(Light::Green));
  REQUIRE_FALSE(sm.CanTransition(Light::Red));
  REQUIRE_FALSE(sm.Transition(Light::Red));
  REQUIRE(sm.Current() == Light::Green);
}

TEST_CASE("AddState is idempotent for an already-known state",
         "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  NodeID first = sm.AddState(Light::Green);
  NodeID second = sm.AddState(Light::Green);
  REQUIRE(first == second);
}

TEST_CASE("HasState reflects states added directly or via AddTransition",
         "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  REQUIRE(sm.HasState(Light::Red));
  REQUIRE_FALSE(sm.HasState(Light::Yellow));

  sm.AddTransition(Light::Red, Light::Yellow);
  REQUIRE(sm.HasState(Light::Yellow));
}

TEST_CASE("Transitions lists the states reachable from the current state",
         "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  sm.AddTransition(Light::Red, Light::Green);
  sm.AddTransition(Light::Red, Light::Yellow);

  std::vector<Light> transitions = sm.Transitions();
  REQUIRE(transitions.size() == 2);
  REQUIRE(std::find(transitions.begin(), transitions.end(), Light::Green) !=
         transitions.end());
  REQUIRE(std::find(transitions.begin(), transitions.end(), Light::Yellow) !=
         transitions.end());
}

TEST_CASE("Transitions is empty when the current state has no outgoing edges",
         "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  REQUIRE(sm.Transitions().empty());
}

TEST_CASE("A full cycle of transitions walks through every state",
         "[ds][state_machine]") {
  StateMachine<Light> sm(Light::Red);
  sm.AddTransition(Light::Red, Light::Green);
  sm.AddTransition(Light::Green, Light::Yellow);
  sm.AddTransition(Light::Yellow, Light::Red);

  REQUIRE(sm.Transition(Light::Green));
  REQUIRE(sm.Transition(Light::Yellow));
  REQUIRE(sm.Transition(Light::Red));
  REQUIRE(sm.Current() == Light::Red);
}
