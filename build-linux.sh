#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS="$ROOT/deps"
BUILD="$ROOT/build"
JOBS="${JOBS:-$(nproc)}"

usage() {
    cat <<'EOF'
Build gw2browser on Linux with CMake.

Prerequisites (Ubuntu/Debian):
  sudo apt install build-essential cmake git pkg-config \
    libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev \
    libwebp-dev libglew-dev libopenal-dev libmpg123-dev \
    libvorbis-dev libogg-dev libfreetype6-dev libtinyxml2-dev libglm-dev

This script also builds wxWidgets 3.2 (required by the project) and the
bundled gw2dattools/gw2formats libraries into ./deps when they are missing.

Usage:
  ./build-linux.sh
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "error: missing command: $1" >&2
        exit 1
    }
}

need_cmd git
need_cmd cmake
need_cmd make

if [[ ! -f "$ROOT/extern/gw2dattools/CMakeLists.txt" ]]; then
    echo "Initializing git submodules..."
    git -C "$ROOT" submodule update --init --recursive
fi

mkdir -p "$DEPS"

build_cmake_subproject() {
    local name="$1"
    local src="$2"
    local marker="$DEPS/lib/cmake/${name}/lib${name}Config.cmake"
    if [[ -f "$marker" ]]; then
        return
    fi
    echo "Building $name..."
    local dir="$src/build"
    cmake -S "$src" -B "$dir" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$DEPS"
    cmake --build "$dir" -j"$JOBS"
    cmake --install "$dir"
}

build_cmake_subproject gw2dattools "$ROOT/extern/gw2dattools"
build_cmake_subproject gw2formats "$ROOT/extern/gw2formats"

wx_config="$(find "$DEPS" -path '*/wx/config/gtk3-unicode-3.2' 2>/dev/null | head -1 || true)"
if [[ -z "$wx_config" ]]; then
    if ! pkg-config --exists gtk+-3.0 2>/dev/null; then
        echo "error: gtk+-3.0 development files not found." >&2
        echo "Install prerequisites with apt (see ./build-linux.sh --help)." >&2
        exit 1
    fi
    echo "Building wxWidgets 3.2..."
    if [[ ! -f "$ROOT/extern/wxWidgets/CMakeLists.txt" ]]; then
        git clone --branch v3.2.4 https://github.com/wxWidgets/wxWidgets.git "$ROOT/extern/wxWidgets"
    fi
    if [[ ! -f "$ROOT/extern/wxWidgets/3rdparty/nanosvg/nanosvg.h" ]]; then
        echo "Initializing wxWidgets submodules..."
        git -C "$ROOT/extern/wxWidgets" submodule update --init --recursive
    fi
    # wxWidgets ships cmake modules in ./build/cmake; never use that as the binary dir.
    wx_build="$ROOT/extern/wxWidgets/build-cmake"
    if [[ -f "$wx_build/CMakeCache.txt" ]] && grep -q 'deps/sysroot' "$wx_build/CMakeCache.txt"; then
        echo "Removing stale wxWidgets build cache (deps/sysroot linker flags)..."
        rm -rf "$wx_build"
    fi
    cmake -S "$ROOT/extern/wxWidgets" -B "$wx_build" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX="$DEPS" \
        -DwxBUILD_SHARED=ON \
        -DwxUSE_OPENGL=ON
    cmake --build "$wx_build" -j"$JOBS"
    cmake --install "$wx_build"
    wx_config="$(find "$DEPS" -path '*/wx/config/gtk3-unicode-3.2' | head -1)"
fi

if [[ -f "$BUILD/CMakeCache.txt" ]] && grep -q 'deps/sysroot' "$BUILD/CMakeCache.txt"; then
    echo "Removing stale gw2browser build cache (deps/sysroot flags)..."
    rm -rf "$BUILD"
fi

mkdir -p "$BUILD"
cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH="$DEPS" \
    -DwxWidgets_CONFIG_EXECUTABLE="$wx_config"

cmake --build "$BUILD" -j"$JOBS"

echo
echo "Build complete: $BUILD/gw2browser"
echo "Run with:"
echo "  LD_LIBRARY_PATH=$DEPS/lib:\$LD_LIBRARY_PATH $BUILD/gw2browser"
