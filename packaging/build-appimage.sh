#!/usr/bin/env bash
# Verso Coder — AppImage paketleyici (Stage 8)
# Kullanım:  ./packaging/build-appimage.sh [build-dir]
set -euo pipefail

BUILD_DIR="${1:-build}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APPDIR="$ROOT/$BUILD_DIR/AppDir"
VERSION="$(sed -n 's/.*version().*return "\([0-9.]*\)".*/\1/p' "$ROOT/src/core/AboutInfo.cpp" | head -1)"
VERSION="${VERSION:-1.0.0}"

echo "==> Derleniyor ($BUILD_DIR)"
cmake -S "$ROOT" -B "$ROOT/$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/$BUILD_DIR" -j"$(nproc)"

echo "==> AppDir hazırlanıyor"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" "$APPDIR/usr/share/metainfo"
install -m755 "$ROOT/$BUILD_DIR/verso-coder" "$APPDIR/usr/bin/"
cp -r "$ROOT/resources" "$APPDIR/usr/bin/resources" 2>/dev/null || true
cp "$ROOT/packaging/verso-coder.desktop" "$APPDIR/usr/share/applications/"
cp "$ROOT/packaging/verso-coder.appdata.xml" "$APPDIR/usr/share/metainfo/"
cp "$ROOT/packaging/verso-coder.desktop" "$APPDIR/verso-coder.desktop"
# ikon yoksa basit bir yer tutucu oluştur (varsa gerçek ikonu kopyala)
if [ -f "$ROOT/resources/icon.png" ]; then
  cp "$ROOT/resources/icon.png" "$APPDIR/verso-coder.png"
  cp "$ROOT/resources/icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/verso-coder.png"
fi

# AppRun
cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
exec "$HERE/usr/bin/verso-coder" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo "==> appimagetool aranıyor"
if command -v appimagetool >/dev/null 2>&1; then
  ARCH="$(uname -m)" appimagetool "$APPDIR" "$ROOT/$BUILD_DIR/VersoCoder-$VERSION-$(uname -m).AppImage"
  echo "==> AppImage üretildi."
else
  echo "!! appimagetool bulunamadı. AppDir hazır: $APPDIR"
  echo "   https://github.com/AppImage/AppImageKit/releases adresinden indirip PATH'e ekleyin."
fi
