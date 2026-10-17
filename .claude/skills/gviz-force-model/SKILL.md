---
name: gviz-force-model
description: Use when adding, modifying, or reviewing a pairwise force system (a "ForceModel") that plugs into gviz's ForceDirectedLayoutAlgorithm — e.g. "add a new force model", "implement a repulsion/attraction scheme like Fruchterman-Reingold/GRIP/Kamada-Kawai", "make a new force law for the layout engine", or any edit under include/layout/physics/force_model/ or include/layout/concept/force_model.hpp. Explains the ForceModel concept contract, the Configuration/IConfiguration pattern, the .hpp/.tpp split, DistanceCalculationSystem conventions, numerical-safety idioms near-coincident points, and how a model gets wired into force_directed and tested.
---

# Adding a ForceModel to gviz

A `ForceModel` is the pluggable force law that `ForceDirectedLayoutAlgorithm<G, F>`
(`include/layout/algorithms/force_directed.hpp`) uses to compute repulsion and
attraction between nodes. Fruchterman-Reingold (vanilla + GRIP variant) in
`include/layout/physics/force_model/fruchterman_reingold.hpp` is the fullest
worked example; `kamada_kawai.hpp` shows a model mid-implementation (its
`VanillaKamadaKawai` is still a stub) — read both, but copy from Fruchterman-
Reingold.

## The contract

`include/layout/concept/force_model.hpp` defines the compile-time concept every
model must satisfy:

```cpp
template <typename F>
concept ForceModel = requires(F forceModel, DenseNodeID denseId, const double* pos, double dbl) {
  { forceModel.RepulsiveTick(denseId, denseId) } -> std::same_as<void>;
  { forceModel.RepulsiveTick(denseId, pos, dbl) } -> std::same_as<void>;
  { forceModel.AttractiveTick(denseId, denseId) } -> std::same_as<void>;
  { forceModel.EdgeLength() } -> std::same_as<double>;
};
```

What each member actually means, from how `force_directed.tpp` calls it:

- **`RepulsiveTick(DenseNodeID v, DenseNodeID u)`** — exact pairwise repulsion
  between two real nodes. Required by the concept and exercised directly in
  `tests/layout/concept/test_force_model.cpp`, but **the current Barnes-Hut
  algorithm in `force_directed.tpp` never calls this overload** — it only
  calls the point+mass one below. Implement it for contract-completeness and
  for any future direct-pairwise use, but don't assume it's your model's hot
  path.
- **`RepulsiveTick(DenseNodeID v, const double* uPos, double uMass)`** — the
  one that actually drives layouts. `ForceDirectedLayoutAlgorithm::RepulsiveTick`
  walks the quadtree and calls this against either a leaf's real node position
  or an internal quadrant's center-of-mass once the Barnes-Hut opening-angle
  test (`theta`) accepts the approximation. Heads up: neither existing
  Fruchterman-Reingold variant actually uses `uMass` in its body today — the
  aggregated-mass scaling classic Barnes-Hut repulsion expects isn't applied.
  Decide deliberately whether your model should use it; don't copy the
  omission by reflex.
- **`AttractiveTick(DenseNodeID v, DenseNodeID u)`** — called once per
  outgoing edge, real node to real node only (attraction isn't Barnes-Hut
  approximated).
- **`EdgeLength() const`** — the model's natural/target spacing. Used once at
  construction by `ForceDirectedLayoutAlgorithm` to size the initial random
  placement bounding box (`boxExtent = 0.5 * sqrt(graph.Size()) *
  forceModel->EdgeLength()`), so it must return a real positive scale that
  matches whatever spacing config field your model exposes (`edgeLength`,
  `targetSpacing`, ...) — not a placeholder.

Both ticks must *add to*, not replace, a node's accumulated displacement — see
"Writing displacement" below.

## File layout and the .hpp/.tpp split

Every model is `template <GraphLike G> class YourModel`, declared in
`include/layout/physics/force_model/<name>.hpp` with implementations in a
sibling `.tpp` included at the bottom of the `.hpp`. End the `.hpp` with a
`static_assert` proving the concept is satisfied for the project's concrete
`Graph` type — this is the project's actual compile-time test for the
contract, do this even before you write a runtime unit test:

```cpp
#include "layout/physics/force_model/your_model.tpp"
static_assert(ForceModel<YourModel<Graph>>);
```

## Class skeleton

```cpp
template <GraphLike G> class YourModel {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double edgeLength = 100.0;
    // ... other tunables, with a one-line comment on anything non-obvious
    // (units, why this default, what breaks near 0) ...

    Configuration WithEdgeLength(double v) { edgeLength = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Edge Length", &edgeLength, ConfigFieldType::Double, 1.0, 2000.0},
      };
    }
  };

  YourModel(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, const double *uPos, double uMass);
  inline double EdgeLength() const { return m_config->edgeLength; }

private:
  Configuration *m_config;
  uint8_t m_dimension;
  DistanceCalculationSystem<G> m_distanceCalc;
  DenseComponentPool<PhysicsComponent> *m_physics;
};
```

Constructor pattern (see `fruchterman_reingold.tpp`):

```cpp
template <GraphLike G>
YourModel<G>::YourModel(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)), m_distanceCalc(graph),
      m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);
}
```

- `RequireConfiguration(config)` (from `config/configuration.hpp`) null-checks
  and throws `MissingConfigurationException<Configuration>` — always route the
  incoming pointer through it rather than storing it raw.
- Fetch component pools you need via `graph.NodeSpace().template
  GetPool<T>()`. Models typically need `PhysicsComponent` (to write
  displacement) and sometimes `PositionComponent` directly (see the pitfall
  below); `DistanceCalculationSystem<G>` wraps `PositionComponent`/
  `RadiusComponent` lookups for you.
- `DimensionResource` must exist on the graph (2D/3D); fetch and validate it
  in the constructor exactly like this, not lazily.

## Configuration pattern

Every model's `Configuration : public IConfiguration` is a plain-data struct
with:
- Public fields with sane defaults, each documented with a one-line comment
  only when the *why* isn't obvious (see `minDistFraction`'s comment in
  `fruchterman_reingold.hpp` for the bar to hit).
- `With<Field>(v)` fluent setters returning `*this` by value, for call-site
  construction (`Configuration().WithEdgeLength(50.0)`).
- `Fields() override` returning `std::vector<ConfigField>` — `{name, &field,
  ConfigFieldType, min, max}` — this is what binds the field to the live
  debug/config UI window, so every tunable a user should be able to drag a
  slider for belongs here, with realistic min/max bounds.

`ForceDirectedLayoutAlgorithm::Configuration` nests your model's config as
`typename F::Configuration forceModelConfig;` and constructs your model with
`std::make_unique<F>(graph, &m_config->forceModelConfig)` — so your
`Configuration` must be default-constructible and copyable with no
constructor arguments.

## Distance helpers and vector convention

Use `DistanceCalculationSystem<G>` (`layout/helpers/distance.hpp`) instead of
hand-rolling position math where you can:

- `BetweenNodes(v, u)` → scalar distance.
- `VecBetweenNodes(a, b, out)` → writes **`b - a`** into `out`, i.e. the
  vector pointing *from `a` to `b`*. Confirm this against call sites, not just
  the header comment, when in doubt — e.g. attraction calls
  `VecBetweenNodes(v, u, out)` (vector toward the neighbor, to pull `v`
  toward it) while repulsion calls `VecBetweenNodes(u, v, out)` (vector away
  from `u`, to push `v` away).
- `VecToPoint(v, p, out)` / `DistanceToPoint(v, p)` — same convention, against
  an arbitrary point (a quadtree center-of-mass) instead of a second node.

## Writing displacement

Both tick methods accumulate into the node's `PhysicsComponent::disp[3]` via
`Vecaxpy(scale, vec, dest, dimension)` (`y += scale * x`, from `ds/vector.hpp`)
— **never** assign/overwrite `disp` directly, since `ForceDirectedLayoutAlgorithm::Tick`
zeroes it once per tick and expects every repulsive + attractive contribution
across the whole tick to sum into it before gravity/heat scaling is applied.

```cpp
Vecaxpy(magnitude, out, m_physics->Find(v.Raw())->disp, m_dimension);
```

## Numerical safety near coincident points

Repulsion blows up as nodes approach the same point (`1/dist^2`). The
codebase uses a strategy , don't skip
this:

1. **Skip-if-zero** (`GRIPFruchtermanReingold`): if the direction vector is
   exactly zero (`IsZero(out, m_dimension)`), skip the tick entirely. Simpler,
   but two exactly-coincident nodes stay stuck until something else (initial
   randomized placement) separates them.

## Pitfall: don't copy a `PositionComponent` by value

`PositionComponent::pos` is declared `double[1]` but is really backed by an
`m_dimension`-wide slice of shared storage (`PositionSpan`), valid only
through a pointer into that storage. A by-value copy (`auto p = *ptr`) only
copies `pos[0]`, leaving every axis past X as uninitialized stack garbage —
this was a real bug that silently corrupted every axis but X in the
Barnes-Hut repulsion hot path. When you need a node's own position vector,
keep a pointer:

```cpp
const PositionComponent *vPos = m_positions->Find(v.Raw());
// use vPos->pos[i], never `auto vPos = *m_positions->Find(...)`
```

## Wiring it in and testing

1. `static_assert(ForceModel<YourModel<Graph>>);` at the bottom of your
   `.hpp` (see "File layout" above) — this is the fast, always-on check.
2. Add it to `tests/layout/concept/test_force_model.cpp`'s
   `STATIC_REQUIRE(ForceModel<YourModel<Graph>>)` alongside the existing
   variants.
3. Plug it into the full algorithm the way `test_force_directed.cpp` does, to
   exercise construction/wiring end to end:
   ```cpp
   ForceDirectedLayoutAlgorithm<Graph, YourModel<Graph>>::Configuration algConfig;
   ForceDirectedLayoutAlgorithm<Graph, YourModel<Graph>> alg(g, &algConfig);
   ```
   Use `test_helpers::MakeGraph(n, dim)` for the graph fixture. Note
   `force_directed.tpp`'s `Tick()` currently segfaults (the quadtree root is
   never populated before use — see the top-of-file comment in
   `test_force_directed.cpp`), so today's tests stop at construction/placement
   checks, not a full `Tick()` call.
4. Add `tests/layout/physics/test_<name>.cpp` (flat, alongside
   `test_fruchterman_reingold.cpp`, `test_kamada_kawai.cpp`,
   `test_grip_physics.cpp` — not a `force_model/` subdirectory) for your
   model's own tick math: verify `AttractiveTick` pulls two nodes together,
   `RepulsiveTick` pushes them apart, near-zero distance doesn't NaN. Build
   the fixture with `tests/helpers/graph_helpers.hpp`'s `MakeGraph`,
   `WithPositions`/`WithPhysics`/`WithRadius`, and `SetPosition` rather than
   wiring pools by hand — see `test_fruchterman_reingold.cpp` for the
   pattern. No manual CMake registration needed — `tests/CMakeLists.txt`
   globs `*.cpp` recursively.

   Exercise **all three** tick overloads in tests, including the point+mass
   one — templates only type-check members they instantiate, so a broken
   overload can sit uncompiled and undetected otherwise.
   `GRIPFruchtermanReingold::RepulsiveTick(v, pos, mass)` has exactly this
   bug today (`test_fruchterman_reingold.cpp`'s header comment: it calls a
   `m_distanceCalc.ToPoint(...)` member that doesn't exist, only
   `DistanceToPoint` does) and is silently excluded from the test suite as a
   result — don't let your new model's equivalent overload go untested the
   same way.
5. Build and run: `cmake --build build && ./build/tests/gviz_tests` (adjust
   build dir to whichever of `build`/`build-release`/`build-render` you're
   using).
