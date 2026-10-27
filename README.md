# gviz

gviz is a graph visualizer. It is **not** the engine — it's the end-user
application (layout algorithms, renderer, debug tooling) built on top of a
small two-part engine:

- **ECS** (`IndexSpace`) — generic sparse/dense entity-component storage.
- **Renderer** — consumes component data and draws it.

A `Graph` (or `Subgraph`) owns two private `IndexSpace`s, one for nodes and
one for edges, and uses them only to store its own adjacency/degree/weight
data as components — that's graph data, reachable through `Graph`'s own
methods (`OutNeighbors`, `Degree`, `GetEdge`, `GetWeight`, ...), never through
a raw pool handed out to callers. Every other piece of per-node/per-edge data
a layout algorithm or the renderer needs (position, color, physics state,
curvature, ...) lives in a separate `DecoratedGraph<G>`: a small struct a
layout algorithm owns, builds from the graph it's given, and hands out so the
renderer (or another consumer) can read what it produced. Because the
decoration is a separate object from the graph, many `DecoratedGraph`s —
even several of the same algorithm — can exist over the same `Graph`
simultaneously without copying its adjacency data.

## The ECS

The ECS is built up from a few small layers, each one built on the last:

1. **`IndexSpace`** — maps a sparse `EntityID` (e.g. a `NodeID`/`EdgeID`) to a
   dense index and back, and owns one `DenseComponentPool<T>` per component
   type `T` registered on it. `Graph`/`Subgraph` each keep a private node
   `IndexSpace` and edge `IndexSpace` purely for their own adjacency/degree/
   weight pools; `DecoratedGraph<G>` (below) owns a completely separate pair
   for everything else, so a node and an edge — and a graph's own data vs. a
   decoration's — can have same-named component types without colliding.
2. **`ComponentPool`** (`DenseComponentPool<T>`) — a flat `std::vector<T>`
   keyed by dense index, auto-grown whenever its `IndexSpace` creates a new
   entity. This is the only place component data physically lives.
3. **`Component`** — just a plain struct, e.g. `PositionComponent`,
   `RadiusComponent`, `WeightComponent`. A component has no behavior; it's a
   row of data in a pool.
4. **`DecoratedGraph<G>`** (`include/graph/decorated_graph.hpp`) — the object
   a layout algorithm builds from the `G &graph` it's given: a `nodeSpace`
   and `edgeSpace` sized and ID-mapped 1:1 with the graph (via `Init`), a
   `dimension`, a pointer back to the graph, and a resource map
   (`SetResource`/`GetResource`/`HasResource`) for whatever other per-run
   state a system needs (`NestedFiltrationResult`, `BFSScratch`,
   `QuadTreeResource`, ...). It's the only place decorator component pools
   and run-scoped resources live — `Graph`/`Subgraph` themselves expose
   neither.
5. **`System`** — a class handed a `DecoratedGraph<G>&` (plus the raw graph
   pointer it carries, for adjacency), that pulls out pointers to the
   specific pools/resources it needs and implements one piece of
   functionality over that subset. For example `RenderQuerySystem` only
   touches `PositionComponent`, `RadiusComponent`, `ColorComponent`,
   `CubicBezierComponent`, and `HalfWidthComponent`; `GravityForceSystem`
   only touches `PositionComponent` and `MassComponent`. Systems are the
   layer where actual logic lives.
6. **Layout algorithm** — the top-level, user-facing class (e.g.
   `ForceDirectedLayoutAlgorithm`, `GRIPLayoutAlgorithm`,
   `SankeyLayoutAlgorithm`). Constructed from `(G &graph, Configuration*)`,
   it builds its own `DecoratedGraph<G>`, creates whatever component pools
   its systems need on it, owns and orchestrates those systems in the right
   order on each `Tick()`, and exposes a small, stable public API (`Tick()`,
   `Layout()`, `Config()`, `Decorated()`, ...) so the caller never has to know
   which systems or components exist underneath. Because the algorithm owns
   its `DecoratedGraph` rather than attaching data to the graph itself,
   several algorithms — even several instances of the same one — can run
   concurrently over the same `Graph` without copying its adjacency data;
   `Decorated()` is how a caller gets at the result to render it or add more
   decorations (e.g. `RadiusComponent`/`ColorComponent` for the renderer).

Graphs conform to the `GraphLike` concept (`include/concept/graphLike.hpp`),
so systems and layout algorithms are templated on `G` and work identically
over `Graph` or `Subgraph`.

## The Renderer

At its simplest, the renderer only cares about one thing: every node's
`PositionComponent`. Each frame, `Renderer::Frame(decoratedGraph)` runs a
`RenderQuerySystem<G>` over it, which walks `decoratedGraph.graph->Nodes()` /
`Edges()`, reads `PositionComponent` (plus optional `RadiusComponent` and
`ColorComponent`) from `decoratedGraph.nodeSpace`/`edgeSpace` for nodes, and
`EdgeComponent`/adjacency from the graph itself for edges, and packs
everything into a flat `FrameData` (positions, radii, colors, edge index
pairs, bounding box) that gets handed to the GPU backend. The renderer never
sees a raw `Graph`/`Subgraph` — only the `DecoratedGraph<G>` a layout
algorithm produced (via its `Decorated()` accessor). If that decoration has
no `PositionComponent` pool at all, the renderer draws nothing.

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

1. **Reingold-Tilford layout.** Port the classic Reingold-Tilford tree
   layout algorithm, with step-by-step animation of its separation and
   contour-comparison events.
2. **Interactivity.** Click-to-pick a vertex, inspect its data, and highlight
   its in/out neighbors in the rendered view.
3. **Explicit graph classification.** A clear, queryable distinction between
   weighted/unweighted and directed/undirected graphs — likely a `uint8_t`
   flags field on `Graph`/`Subgraph` that algorithms can branch on.
4. **`.gexf` import.** Parse `.gexf` files into a `DecoratedGraph`, using the
   decoration to carry `.gexf`-specific viz elements; this may require
   layout algorithm constructors to accept a `DecoratedGraph` directly.
5. **End-to-end GUI.** Possibly a full application shell around the
   renderer, rather than code-driven demos.
6. **GUI widget system.** A widget is the renderer-side analog of a System:
   it consumes a subset of component pools and/or resources and produces a
   rendered widget as output, instead of producing derived component data.
   Widgets would be orchestrated and composed through the renderer API, so
   a user can assemble whatever scene (inspector panels, legends, charts,
   controls, ...) they want out of them, the same way a layout algorithm
   composes systems.
