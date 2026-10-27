const BEZIER_CLOSEST_SAMPLES : u32 = 8u;
const BEZIER_NEWTON_ITERS : u32 = 4u;

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

fn evalBezierAccel(curveIdx : u32, t : f32) -> vec3f {
  let p0 = getControlPoint(curveIdx, 0u);
  let p1 = getControlPoint(curveIdx, 1u);
  let p2 = getControlPoint(curveIdx, 2u);
  let p3 = getControlPoint(curveIdx, 3u);
  let mt = 1.0 - t;
  return 6.0 * mt * (p2 - 2.0 * p1 + p0) + 6.0 * t * (p3 - 2.0 * p2 + p1);
}

fn closestTOnBezier(curveIdx : u32, p : vec3f) -> f32 {
  var bestT = 0.0;
  var bestDist = 3.4e38;
  for (var i = 0u; i <= BEZIER_CLOSEST_SAMPLES; i = i + 1u) {
    let t = f32(i) / f32(BEZIER_CLOSEST_SAMPLES);
    let d = length(evalBezier(curveIdx, t) - p);
    if (d < bestDist) {
      bestDist = d;
      bestT = t;
    }
  }

  var t = bestT;
  for (var i = 0u; i < BEZIER_NEWTON_ITERS; i = i + 1u) {
    let b = evalBezier(curveIdx, t);
    let d1 = evalBezierTangent(curveIdx, t);
    let d2 = evalBezierAccel(curveIdx, t);
    let diff = b - p;
    let f = dot(diff, d1);
    let fp = dot(d1, d1) + dot(diff, d2);
    if (abs(fp) > 1e-8) {
      t = clamp(t - f / fp, 0.0, 1.0);
    }
  }
  return t;
}

struct BezierOut {
  @builtin(position) clip : vec4f,
  @location(0) worldPos : vec3f,
  @location(1) @interpolate(flat) curveIdx : u32,
  @location(2) @interpolate(flat) halfWidth : f32,
  @location(3) @interpolate(flat) color : vec4f,
}

@vertex
fn vsBezier(@builtin(vertex_index) vid : u32,
           @builtin(instance_index) iid : u32) -> BezierOut {
  let corner = CORNERS[vid];
  let p0 = getControlPoint(iid, 0u);
  let p1 = getControlPoint(iid, 1u);
  let p2 = getControlPoint(iid, 2u);
  let p3 = getControlPoint(iid, 3u);
  let minP = min(min(p0, p1), min(p2, p3));
  let maxP = max(max(p0, p1), max(p2, p3));

  let halfWidth = halfWidths[iid];
  let pad = halfWidth + G.worldPerPixel;
  let center = (minP + maxP) * 0.5;
  let halfExtent = (maxP - minP) * 0.5 + vec3f(pad, pad, 0.0);

  let worldPos = vec3f(center.x + corner.x * halfExtent.x,
                       center.y + corner.y * halfExtent.y, center.z);
  let clip = G.viewProj * vec4f(worldPos, 1.0);
  let color = unpackColor(edgeColors[iid]);
  return BezierOut(clip, worldPos, iid, halfWidth, color);
}

@fragment
fn fsBezier(in : BezierOut) -> @location(0) vec4f {
  let aa = max(G.worldPerPixel, 1e-6);

  let t = closestTOnBezier(in.curveIdx, in.worldPos);
  let closest = evalBezier(in.curveIdx, t);
  let dist = length(in.worldPos - closest);
  let widthCoverage = 1.0 - smoothstep(in.halfWidth - aa, in.halfWidth + aa, dist);

  let p0 = getControlPoint(in.curveIdx, 0u);
  let p3 = getControlPoint(in.curveIdx, 3u);
  let tangent0 = evalBezierTangent(in.curveIdx, 0.0);
  let tangent1 = evalBezierTangent(in.curveIdx, 1.0);
  let dir0 = tangent0 / max(length(tangent0), 1e-6);
  let dir1 = tangent1 / max(length(tangent1), 1e-6);
  let capCoverage0 = smoothstep(-aa, aa, dot(in.worldPos - p0, dir0));
  let capCoverage1 = smoothstep(-aa, aa, dot(p3 - in.worldPos, dir1));

  let coverage = widthCoverage * capCoverage0 * capCoverage1;
  if (coverage <= 0.0) {
    discard;
  }
  return vec4f(in.color.rgb, in.color.a * coverage);
}
