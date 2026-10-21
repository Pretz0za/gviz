struct EdgeOut {
  @builtin(position) clip : vec4f,
  @location(0) across : f32,
  @location(1) @interpolate(flat) halfWidth : f32,
  @location(2) @interpolate(flat) color : vec4f,
}

@vertex
fn vsEdge(@builtin(vertex_index) vid : u32,
          @builtin(instance_index) iid : u32) -> EdgeOut {
  let a = edges[iid * 2u];
  let b = edges[iid * 2u + 1u];
  var clipA = G.viewProj * vec4f(getPos(a), 1.0);
  var clipB = G.viewProj * vec4f(getPos(b), 1.0);
  clipA.w = max(clipA.w, 1e-4);
  clipB.w = max(clipB.w, 1e-4);

  let screenA = clipA.xy / clipA.w * G.viewport * 0.5;
  let screenB = clipB.xy / clipB.w * G.viewport * 0.5;
  var dir = screenB - screenA;
  let len = length(dir);
  if (len < 1e-6) { dir = vec2f(1.0, 0.0); } else { dir = dir / len; }
  let normal = vec2f(-dir.y, dir.x);

  var end = 0.0;
  if (vid == 1u || vid == 2u || vid == 4u) { end = 1.0; }
  var side = -1.0;
  if (vid == 2u || vid == 4u || vid == 5u) { side = 1.0; }

  let clipEnd = mix(clipA, clipB, end);
  let halfWidth = halfWidths[iid] * 0.5 + 0.5;
  let screen = mix(screenA, screenB, end) + normal * side * halfWidth;
  let clip = vec4f(screen * 2.0 / G.viewport * clipEnd.w, clipEnd.zw);
  var color = G.edgeColor;
  if (G.useEdgeColors != 0u) { color = unpackColor(edgeColors[iid]); }
  return EdgeOut(clip, side * halfWidth, halfWidth, color);
}

@fragment
fn fsEdge(in : EdgeOut) -> @location(0) vec4f {
  let coverage = 1.0 - smoothstep(in.halfWidth - 0.5, in.halfWidth + 0.5, abs(in.across));
  if (coverage <= 0.0) { discard; }
  return vec4f(in.color.rgb, in.color.a * coverage);
}
