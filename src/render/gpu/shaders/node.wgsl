const CORNERS = array<vec2f, 6>(
  vec2f(-1.0, -1.0), vec2f(1.0, -1.0), vec2f(1.0, 1.0),
  vec2f(-1.0, -1.0), vec2f(1.0, 1.0), vec2f(-1.0, 1.0));

struct NodeOut {
  @builtin(position) clip : vec4f,
  @location(0) local : vec2f,
  @location(1) @interpolate(flat) halfExtent : vec2f,
  @location(2) @interpolate(flat) isRect : f32,
  @location(3) @interpolate(flat) color : vec4f,
}

@vertex
fn vsNode(@builtin(vertex_index) vid : u32,
          @builtin(instance_index) iid : u32) -> NodeOut {
  let corner = CORNERS[vid];
  let id = nodeIds[iid];
  var halfExtent = vec2f(radii[id], radii[id]);
  var isRect = 0.0;
  let rectHalfHeight = nodeRectHalfHeights[id];
  if (rectHalfHeight > 0.0) {
    halfExtent.y = rectHalfHeight;
    isRect = 1.0;
  }
  let aa = G.worldPerPixel;
  let quadWorld = halfExtent + vec2f(aa, aa);
  let local = corner * quadWorld;
  let worldPos = getPos(id) + G.cameraRight * local.x + G.cameraUp * local.y;
  let clip = G.viewProj * vec4f(worldPos, 1.0);
  return NodeOut(clip, local, halfExtent, isRect, unpackColor(colors[id]));
}

@fragment
fn fsNode(in : NodeOut) -> @location(0) vec4f {
  let aa = max(G.worldPerPixel, 1e-6);
  var sdf : f32;
  if (in.isRect > 0.5) {
    let d = abs(in.local) - in.halfExtent;
    sdf = max(d.x, d.y);
  } else {
    sdf = length(in.local) - in.halfExtent.x;
  }
  let coverage = 1.0 - smoothstep(-aa, aa, sdf);
  if (coverage <= 0.0) { discard; }
  return vec4f(in.color.rgb, in.color.a * coverage);
}
