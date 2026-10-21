const BEZIER_SEGMENTS : u32 = 24u;

fn getControlPoint(curveIdx : u32, i : u32) -> vec3f {
  let base = curveIdx * 12u + i * 3u;
  return vec3f(bezierControlPoints[base], bezierControlPoints[base + 1u],
               bezierControlPoints[base + 2u]);
}

fn evalBezier(curveIdx : u32, t : f32) -> vec3f {
  let p0 = getControlPoint(curveIdx, 0u);
  let p1 = getControlPoint(curveIdx, 1u);
  let p2 = getControlPoint(curveIdx, 2u);
  let p3 = getControlPoint(curveIdx, 3u);
  let mt = 1.0 - t;
  return mt * mt * mt * p0 + 3.0 * mt * mt * t * p1 + 3.0 * mt * t * t * p2 +
         t * t * t * p3;
}

fn evalBezierTangent(curveIdx : u32, t : f32) -> vec3f {
  let p0 = getControlPoint(curveIdx, 0u);
  let p1 = getControlPoint(curveIdx, 1u);
  let p2 = getControlPoint(curveIdx, 2u);
  let p3 = getControlPoint(curveIdx, 3u);
  let mt = 1.0 - t;
  return 3.0 * mt * mt * (p1 - p0) + 6.0 * mt * t * (p2 - p1) +
         3.0 * t * t * (p3 - p2);
}

fn bezierWorldNormal(curveIdx : u32, t : f32) -> vec3f {
  let tangent = evalBezierTangent(curveIdx, t);
  var dir = vec2f(dot(tangent, G.cameraRight), dot(tangent, G.cameraUp));
  let len = length(dir);
  if (len < 1e-6) { dir = vec2f(1.0, 0.0); } else { dir = dir / len; }
  let n = vec2f(-dir.y, dir.x);
  return G.cameraRight * n.x + G.cameraUp * n.y;
}

struct BezierSegmentOut {
  @builtin(position) clip : vec4f,
  @location(0) across : f32,
  @location(1) @interpolate(flat) halfWidth : f32,
  @location(2) @interpolate(flat) color : vec4f,
}

@vertex
fn vsBezierSegment(@builtin(vertex_index) vid : u32,
                   @builtin(instance_index) iid : u32) -> BezierSegmentOut {
  let curveIdx = iid;
  let segIdx = vid / 2u;
  var side = -1.0;
  if (vid % 2u == 1u) { side = 1.0; }
  let t = f32(segIdx) / f32(BEZIER_SEGMENTS);

  let center = evalBezier(curveIdx, t);
  let normal = bezierWorldNormal(curveIdx, t);
  let coreHalfWidth = halfWidths[curveIdx];
  let aa = G.worldPerPixel;
  let offset = side * (coreHalfWidth + aa);
  let worldPos = center + normal * offset;
  let clip = G.viewProj * vec4f(worldPos, 1.0);
  let color = unpackColor(edgeColors[curveIdx]);
  return BezierSegmentOut(clip, offset, coreHalfWidth, color);
}

@fragment
fn fsBezierSegment(in : BezierSegmentOut) -> @location(0) vec4f {
  let aa = max(G.worldPerPixel, 1e-6);
  let coverage = 1.0 - smoothstep(in.halfWidth - aa, in.halfWidth + aa, abs(in.across));
  if (coverage <= 0.0) { discard; }
  return vec4f(in.color.rgb, in.color.a * coverage);
}
