struct Globals {
  viewProj      : mat4x4f,
  viewport      : vec2f,
  worldPerPixel : f32,
  nodeColor     : vec4f,
  edgeColor     : vec4f,
  cameraRight   : vec3f,
  cameraUp      : vec3f,
  useEdgeColors : u32,
}

@group(0) @binding(0) var<uniform> G : Globals;
@group(0) @binding(1) var<storage, read> positions  : array<f32>;
@group(0) @binding(2) var<storage, read> nodeIds    : array<u32>;
@group(0) @binding(3) var<storage, read> edges      : array<u32>;
@group(0) @binding(4) var<storage, read> radii      : array<f32>;
@group(0) @binding(5) var<storage, read> colors     : array<u32>;
@group(0) @binding(6) var<storage, read> edgeColors : array<u32>;
@group(0) @binding(7) var<storage, read> bezierControlPoints : array<f32>;
@group(0) @binding(8) var<storage, read> halfWidths : array<f32>;
@group(0) @binding(9) var<storage, read> nodeRectHalfHeights : array<f32>;

fn getPos(i : u32) -> vec3f {
  return vec3f(positions[i * 3u], positions[i * 3u + 1u], positions[i * 3u + 2u]);
}

fn unpackColor(c : u32) -> vec4f {
  return vec4f(f32((c >> 24u) & 0xFFu), f32((c >> 16u) & 0xFFu),
               f32((c >> 8u) & 0xFFu), f32(c & 0xFFu)) / 255.0;
}
