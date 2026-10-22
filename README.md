# gviz

gviz is a graph visualizer. It is **not** the engine — it's the end-user
application (layout algorithms, renderer, debug tooling) built on top of a
small two-part engine:

- **ECS** (`IndexSpace`) — generic sparse/dense entity-component storage.
- **Renderer** — consumes component data and draws it.

A `Graph` (or `Subgraph`) owns two `IndexSpace`s, one for nodes and one for
edges, and uses them to store adjacency and weight data as components. Every
other piece of per-node/per-edge data (position, color, physics state,
curvature, ...) lives in the same spaces as additional component pools.

## The ECS

The ECS is built up from a few small layers, each one built on the last:

1. **`IndexSpace`** — maps a sparse `EntityID` (e.g. a `NodeID`/`EdgeID`) to a
   dense index and back, and owns one `DenseComponentPool<T>` per component
   type `T` registered on it. Nodes and edges each get their own
   `IndexSpace` (`Graph::NodeSpace()` / `Graph::EdgeSpace()`), so a node and
   an edge can have same-named component types without colliding.
2. **`ComponentPool`** (`DenseComponentPool<T>`) — a flat `std::vector<T>`
   keyed by dense index, auto-grown whenever its `IndexSpace` creates a new
   entity. This is the only place component data physically lives.
3. **`Component`** — just a plain struct, e.g. `PositionComponent`,
   `RadiusComponent`, `WeightComponent`. A component has no behavior; it's a
   row of data in a pool.
4. **`System`** — a class that is handed a `GraphLike` graph, pulls out
   pointers to the specific pools it needs (via `IndexSpace::GetPool<T>()`),
   and implements one piece of functionality over that subset of pools. For
   example `RenderQuerySystem` only touches `PositionComponent`,
   `RadiusComponent`, `ColorComponent`, `CubicBezierComponent`, and
   `HalfWidthComponent`; `GravityForceSystem` only touches `PositionComponent`
   and `MassComponent`. Systems are the layer where actual logic lives.
5. **Layout algorithm** — the top-level, user-facing class (e.g.
   `ForceDirectedLayoutAlgorithm`, `GRIPLayoutAlgorithm`,
   `SankeyLayoutAlgorithm`). It creates whatever component pools its systems
   need, owns and orchestrates those systems in the right order on each
   `Tick()`, and exposes a small, stable public API (`Tick()`, `Layout()`,
   `Config()`, ...) so the caller never has to know which systems or
   components exist underneath.

Graphs conform to the `GraphLike` concept (`include/concept/graphLike.hpp`),
so systems and layout algorithms are templated on `G` and work identically
over `Graph` or `Subgraph`.

## The Renderer

At its simplest, the renderer only cares about one thing: every node's
`PositionComponent`. Each frame, `Renderer::Frame(graph)` runs a
`RenderQuerySystem<G>` over the graph, which walks `graph.Nodes()` /
`graph.Edges()`, reads `PositionComponent` (plus optional `RadiusComponent`
and `ColorComponent`) for nodes, and `EdgeComponent`/adjacency for edges, and
packs everything into a flat `FrameData` (positions, radii, colors, edge
index pairs, bounding box) that gets handed to the GPU backend. If a graph
has no `PositionComponent` pool at all, the renderer draws nothing.

### Special cases

The renderer is otherwise layout-agnostic — it doesn't know which layout
algorithm produced the positions it's drawing. It detects extra, *optional*
rendering behavior purely by checking whether a given component pool exists
and is populated, never by branching on which layout algorithm is running:

- **Bezier edges.** If an edge has an entry in the `CubicBezierComponent`
  pool, and that entry's control points aren't all zero
  (`IsCurveSet`), the edge is drawn as a cubic bezier curve instead of a
  straight line. Straight-line layouts simply never populate this pool.
- **Ribbon/flow edges.** If a `HalfWidthComponent` pool exists, edges are
  drawn with per-edge thickness (used by `SankeyLayoutAlgorithm` for
  flow-proportional ribbons); otherwise a flat default width is used.
- **Node visibility.** An optional `VisibleNodesResource` (a dense bitset)
  lets a layout algorithm hide nodes it hasn't placed yet — used by GRIP's
  layered/staged placement — without removing them from the graph.
- **Debug charts** (`GVIZ_DEBUG_CHARTS` build). A `ChartRecorder` can be
  attached to any `IndexSpace` and asked to snapshot a component pool every
  tick (`PushFrame<T>()`). `ChartWindow`/`ChartOverlay` then plot those
  samples live inside the renderer, which is how physics/heat values
  (temperature, kinetic energy, etc.) get inspected without instrumenting
  the algorithm itself.

## Implemented layout algorithms

| Algorithm | Best for | How it works |
|---|---|---|
| `ForceDirectedLayoutAlgorithm` | General, medium-to-large graphs | Classic force-directed simulation with Barnes-Hut (quadtree) repulsion approximation, gravity, and a pluggable `ForceModel` (`FruchtermanReingold`, `LinLog`, `KamadaKawai`) chosen at compile time. |
| `GRIPLayoutAlgorithm` | Large, sparse graphs | Implements GRIP (Gajer & Kobourov): builds a nested MIS (maximal-independent-set) filtration into coarser layers, places each layer by barycentric interpolation from the coarser one, then refines locally with k-nearest-neighbor Fruchterman-Reingold/Kamada-Kawai — scales to graphs too large for naive force-directed layout. |
| `SpringLayoutAlgorithm` | Small/medium graphs, interactive scenes | Direct mass-spring-damper simulation over edges, with node pinning (`Fix`/`Unfix`/`SetPosition`) for interactive dragging. |
| `SankeyLayoutAlgorithm` | Weighted, (acyclic) directed flow graphs | Assigns nodes to layers/columns, orders them within a layer, and sizes/curves edges proportional to `WeightComponent` to draw a classic Sankey flow diagram. |
| `TreeConvexLayoutAlgorithm` | Trees / hierarchies | Places children in an angular wedge around each parent (optionally respecting a given planar embedding), with configurable edge-length modes (uniform, depth-based, subtree-size) and animated transitions between them. |
| `SchnyderLayoutAlgorithm` | Planar (triangulated) graphs | Computes a Schnyder wood (3-coloring/orientation of a planar triangulation via its rotation system) and derives straight-line planar coordinates from the three resulting spanning trees. |

Supporting planarity infrastructure (`PlanarRotationSystem`, Boyer-Myrvold
planarity testing/embedding, `TriangulationSystem`) backs the tree and
Schnyder algorithms but isn't itself a layout algorithm.

## To Be Implemented

1. **Decouple adjacency storage from rendered/decorated data.** Stop
   exposing `NodeSpace()`/`EdgeSpace()` from `GraphLike`. Instead, introduce
   `DecoratedGraph { GraphLike&, IndexSpace nodeSpace, IndexSpace edgeSpace }`,
   where the contained spaces are *separate* from the ones the graph uses
   for adjacency, created and owned by whichever layout algorithm produced
   them. Each layout algorithm exposes a method that outputs its own
   `DecoratedGraph`, and the renderer consumes a `DecoratedGraph` rather than
   a raw graph. This lets multiple layout algorithms run over the same
   underlying connectivity data and each produce an independent,
   simultaneously-renderable result.
2. **Reingold-Tilford layout.** Port the classic Reingold-Tilford tree
   layout algorithm, with step-by-step animation of its separation and
   contour-comparison events.
3. **Interactivity.** Click-to-pick a vertex, inspect its data, and highlight
   its in/out neighbors in the rendered view.
4. **Explicit graph classification.** A clear, queryable distinction between
   weighted/unweighted and directed/undirected graphs — likely a `uint8_t`
   flags field on `Graph`/`Subgraph` that algorithms can branch on.
5. **`.gexf` import.** Parse `.gexf` files into a `DecoratedGraph`, using the
   decoration to carry `.gexf`-specific viz elements; this may require
   layout algorithm constructors to accept a `DecoratedGraph` directly.
6. **End-to-end GUI.** Possibly a full application shell around the
   renderer, rather than code-driven demos.
7. **GUI widget system.** A widget is the renderer-side analog of a System:
   it consumes a subset of component pools and/or resources and produces a
   rendered widget as output, instead of producing derived component data.
   Widgets would be orchestrated and composed through the renderer API, so
   a user can assemble whatever scene (inspector panels, legends, charts,
   controls, ...) they want out of them, the same way a layout algorithm
   composes systems.
