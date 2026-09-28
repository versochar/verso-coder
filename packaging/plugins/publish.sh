#!/bin/bash
# Eklenti yayınlama: yerel .js -> verso-coder-plugins kayıt deposu.
#   publish.sh --check <plugin.js>                 # yalnız doğrula
#   publish.sh <plugin.js|dizin> [--desc "..."] [--author AD] [--reindex]
#   publish.sh --unpublish <kimlik>    # kayıttan çıkarır (denetim)
# Gerekli: git, python3. Push için VERSO_GIT_TOKEN (içerik) ya da
# VERSO_GIT_TOKEN_FILE (dosya yolu) gerekir.
set -u
export LC_ALL=C # [a-z] aralıkları tr_TR'te bozulur
KNOWN="fs.read fs.write events ui net"
REPO_URL="https://github.com/versochar/verso-coder-plugins.git"
REG_DIR="${VERSO_PLUGIN_REPO:-$HOME/verso-coder-plugins}"
MAX_BYTES=204800

die() { echo "hata: $*" >&2; exit 1; }

# --- doğrulama: <dosya> <kimlik> ---
check_plugin() {
    local f="$1" id="$2"
    [ -f "$f" ] || die "$f yok"
    [ "$(stat -c%s "$f")" -le "$MAX_BYTES" ] || die "$f 200KB üstü"
    grep -qE '^[[:space:]]*//[[:space:]]*@name[[:space:]]+.+' "$f" \
        || die "@name başlığı yok"
    grep -qE '^[[:space:]]*//[[:space:]]*@version[[:space:]]+[0-9]+\.[0-9]+(\.[0-9]+)?[[:space:]]*$' "$f" \
        || die "@version X.Y[.Z] olmalı"
    local bad=""
    while IFS= read -r line; do
        for tok in $line; do
            tok="${tok//,/ }"
            for t in $tok; do
                case " $KNOWN " in
                    *" $t "*) ;;
                    *) bad="$bad $t" ;;
                esac
            done
        done
    done < <(grep -E '^[[:space:]]*//[[:space:]]*@permission[[:space:]]+' "$f" \
        | sed -E 's/^[[:space:]]*\/\/[[:space:]]*@permission[[:space:]]+//')
    [ -z "$bad" ] || die "bilinmeyen izinler:$bad (bilinen: $KNOWN)"
    echo "tamam: $id ($(grep -E '^[[:space:]]*//[[:space:]]*@version' "$f" \
        | sed -E 's/.*@version[[:space:]]+//'))"
}

# --- kayıt deposunu hazırla ---
ensure_repo() {
    if [ ! -d "$REG_DIR/.git" ]; then
        git clone -q "$REPO_URL" "$REG_DIR" || die "klonlanamadı"
    fi
    git -C "$REG_DIR" pull -q --rebase 2>/dev/null || true
}

# --- index.json yeniden üret (mevcut açıklama/yazar korunur) ---
reindex() {
    python3 - "$REG_DIR" << 'PYEOF'
import json, os, re, sys

reg = sys.argv[1]
idx_path = os.path.join(reg, "index.json")
old = {}
try:
    old = {p["id"]: p for p in json.load(open(idx_path))["plugins"]}
except Exception:
    pass
known = {"fs.read", "fs.write", "events", "ui", "net"}
out = []
plugdir = os.path.join(reg, "plugins")
for pid in sorted(os.listdir(plugdir)):
    f = os.path.join(plugdir, pid, "plugin.js")
    if not os.path.isfile(f):
        continue
    src = open(f, encoding="utf-8").read()
    name = re.search(r"(?m)^\s*//\s*@name\s+(.+)$", src)
    ver = re.search(r"(?m)^\s*//\s*@version\s+(.+)$", src)
    if not name or not ver:
        print("atlandı (başlıksız):", pid)
        continue
    perms = sorted({t.strip() for line in
                    re.findall(r"(?m)^\s*//\s*@permission\s+(.+)$", src)
                    for t in re.split(r"[\s,]+", line) if t.strip()})
    if any(p not in known for p in perms):
        print("atlandı (bilinmeyen izin):", pid)
        continue
    prev = old.get(pid, {})
    out.append({
        "id": pid,
        "name": name.group(1).strip()[:80],
        "version": ver.group(1).strip()[:40],
        "description": prev.get("description", ""),
        "author": prev.get("author", "versochar"),
        "permissions": perms,
        "file": "plugins/%s/plugin.js" % pid,
        "minApp": prev.get("minApp", ""),
    })
from datetime import date
json.dump({"registry": 1, "updated": str(date.today()), "plugins": out},
          open(idx_path, "w", encoding="utf-8"),
          ensure_ascii=False, indent=2)
open(idx_path, "a").write("\n")
print("index: %d eklenti" % len(out))
PYEOF
}

DESC=""
AUTHOR="versochar"
DO_REINDEX=0
SRC=""

while [ $# -gt 0 ]; do
    case "$1" in
        --desc=*) DESC="${1#--desc=}"; shift ;;
        --desc) DESC="${2:-}"; shift 2 ;;
        --author=*) AUTHOR="${1#--author=}"; shift ;;
        --author) AUTHOR="${2:-}"; shift 2 ;;
        --reindex) DO_REINDEX=1; shift ;;
        --unpublish) DO_REINDEX=3; shift ;;
        --check) DO_REINDEX=2; shift ;;
        -*) die "bilinmeyen seçenek: $1" ;;
        *) SRC="$1"; shift ;;
    esac
done

if [ "$DO_REINDEX" = "2" ]; then
    [ -n "$SRC" ] || die "kullanım: publish.sh --check <plugin.js>"
    ID="$(basename "$SRC" .js)"
    [[ "$ID" =~ ^[a-z0-9][a-z0-9-]{0,39}$ ]] || die "geçersiz kimlik: $ID"
    check_plugin "$SRC" "$ID"
    exit 0
fi

push_repo() { # $1 = commit iletisi
    git -C "$REG_DIR" add -A
    if git -C "$REG_DIR" diff --cached --quiet; then
        echo "değişiklik yok."
        exit 0
    fi
    git -C "$REG_DIR" -c user.name=versochar \
        -c user.email=versochar@users.noreply.github.com \
        commit -qm "$1"
    TOKEN="${VERSO_GIT_TOKEN:-}"
    if [ -z "$TOKEN" ]; then
        TF="${VERSO_GIT_TOKEN_FILE:-$HOME/Masaüstü/git tokens/trhs.txt}"
        [ -f "$TF" ] && TOKEN="$(tr -d '\n\r ' < "$TF")"
    fi
    [ -n "$TOKEN" ] || die "itildi ama push için VERSO_GIT_TOKEN yok (yerel commit duruyor)"
    git -C "$REG_DIR" push -q \
        "https://versochar:${TOKEN}@github.com/versochar/verso-coder-plugins.git" \
        HEAD:main || die "push başarısız"
    git -C "$REG_DIR" remote set-url origin "$REPO_URL" 2>/dev/null || true
}

if [ "$DO_REINDEX" = "3" ]; then
    [ -n "$SRC" ] || die "kullanım: publish.sh --unpublish <kimlik>"
    [[ "$SRC" =~ ^[a-z0-9][a-z0-9-]{0,39}$ ]] || die "geçersiz kimlik: $SRC"
    ensure_repo
    rm -rf "$REG_DIR/plugins/$SRC"
    reindex
    push_repo "eklenti kaldırıldı: $SRC"
    echo "kaldırıldı: $SRC"
    exit 0
fi

[ -n "$SRC" ] || die "kullanım: publish.sh <plugin.js|dizin> [--desc ...] [--author ...]"
JS="$SRC"
[ -d "$SRC" ] && JS="$SRC/plugin.js"
[ -f "$JS" ] || die "$JS yok"
ID="$(basename "$JS" .js)"
[[ "$ID" =~ ^[a-z0-9][a-z0-9-]{0,39}$ ]] || die "geçersiz kimlik: $ID"
check_plugin "$JS" "$ID"

ensure_repo
mkdir -p "$REG_DIR/plugins/$ID"
cp "$JS" "$REG_DIR/plugins/$ID/plugin.js"
reindex
# Yeni girdiyse açıklama/yazar/minApp işle
python3 - "$REG_DIR/index.json" "$ID" "$DESC" "$AUTHOR" << 'PYEOF'
import json, sys
p, pid, desc, author = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
d = json.load(open(p))
for e in d["plugins"]:
    if e["id"] == pid:
        if desc:
            e["description"] = desc[:300]
        if author:
            e["author"] = author[:80]
        if not e["minApp"]:
            e["minApp"] = "1.1.0"
json.dump(d, open(p, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
open(p, "a").write("\n")
PYEOF

git -C "$REG_DIR" add -A
push_repo "eklenti: $ID"
echo "yayınlandı: $ID"
