#!/usr/bin/env bash
# Verso Coder — özel pacman deposu güncelleyici.
# Kullanım: ./packaging/repo/update-repo.sh <paket-dizini>
# Paket dizinindeki *.pkg.tar.zst dosyalarından verso.db üretir.
set -euo pipefail

PKGDIR="${1:-.}"
cd "$PKGDIR"

echo "==> Depo güncelleniyor ($PKGDIR)"
rm -f verso.db verso.files verso.db.tar.gz verso.files.tar.gz
repo-add verso.db.tar.gz ./*.pkg.tar.zst
rm -f verso.db verso.files
ln -s verso.db.tar.gz verso.db
ln -s verso.files.tar.gz verso.files

echo "==> İçerik:"
ls -la verso.db verso.files ./*.pkg.tar.zst
echo
echo "Yükleme: GitHub 'repo' sürümüne verso.db + verso.files + *.pkg.tar.zst ekle."
echo "Kullanıcı tarafı:"
echo "  [verso]"
echo "  SigLevel = Optional TrustAll"
echo "  Server = https://github.com/versochar/verso-coder/releases/download/repo"
