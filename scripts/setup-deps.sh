#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
THIRD_PARTY="$ROOT/third-party"
WGPU_DIR="$THIRD_PARTY/wgpu-native"
GLFW_DIR="$THIRD_PARTY/glfw"

WGPU_NATIVE_VERSION="v29.0.1.1"
GLFW_VERSION="3.4"

case "$(uname -s)" in
Darwin)
    case "$(uname -m)" in
    arm64|aarch64) WGPU_PLATFORM="macos-aarch64" ;;
    *) WGPU_PLATFORM="macos-x86_64" ;;
    esac
    ;;
Linux)
    case "$(uname -m)" in
    arm64|aarch64) WGPU_PLATFORM="linux-aarch64" ;;
    *) WGPU_PLATFORM="linux-x86_64" ;;
    esac
    ;;
MINGW*|MSYS*|CYGWIN*) WGPU_PLATFORM="windows-x86_64-msvc" ;;
*)
    echo "Unsupported OS for automatic wgpu-native download." >&2
    exit 1
    ;;
esac

WGPU_URL="https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_NATIVE_VERSION}/wgpu-${WGPU_PLATFORM}-release.zip"
GLFW_URL="https://github.com/glfw/glfw/releases/download/${GLFW_VERSION}/glfw-${GLFW_VERSION}.zip"

have_wgpu() {
    [[ -f "$WGPU_DIR/include/webgpu/webgpu.h" ]]
}

have_glfw() {
    [[ -f "$GLFW_DIR/CMakeLists.txt" ]]
}

download_and_extract() {
    local url="$1"
    local dest="$2"
    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN

    echo "Downloading $url"
    curl -fsSL "$url" -o "$tmp/archive.zip"
    rm -rf "$dest"
    mkdir -p "$dest"
    unzip -q "$tmp/archive.zip" -d "$tmp/extract"

    local entries
    entries="$(find "$tmp/extract" -mindepth 1 -maxdepth 1)"
    local entryCount
    entryCount="$(echo "$entries" | grep -c .)"
    if [[ "$entryCount" -eq 1 && -d "$entries" ]]; then
        cp -R "$entries/." "$dest/"
    else
        cp -R "$tmp/extract/." "$dest/"
    fi
}

mkdir -p "$THIRD_PARTY"

if ! have_wgpu; then
    download_and_extract "$WGPU_URL" "$WGPU_DIR"
fi

if ! have_glfw; then
    download_and_extract "$GLFW_URL" "$GLFW_DIR"
fi

echo "Dependencies ready:"
echo "  wgpu-native -> $WGPU_DIR"
echo "  GLFW        -> $GLFW_DIR"
