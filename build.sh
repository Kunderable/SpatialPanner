#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "==> Configuring..."
cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -G "Unix Makefiles"

echo "==> Building..."
cmake --build "$BUILD_DIR" --config Release --parallel "$(sysctl -n hw.logicalcpu)"

echo ""
echo "==> Done. Artifacts:"
find "$BUILD_DIR" -name "*.vst3" -o -name "*.component" -o -name "*.app" 2>/dev/null | \
  grep -v "/_deps/" | sort

echo ""
echo "To install VST3:"
echo "  cp -r \"\$(find $BUILD_DIR -name '*.vst3' | grep -v _deps | head -1)\" ~/Library/Audio/Plug-Ins/VST3/"
echo ""
echo "To install AU:"
echo "  cp -r \"\$(find $BUILD_DIR -name '*.component' | grep -v _deps | head -1)\" ~/Library/Audio/Plug-Ins/Components/"
