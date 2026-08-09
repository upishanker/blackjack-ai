#!/usr/bin/env bash
#
# Builds the static, self-contained version of the demo into docs/, which is
# what GitHub Pages serves. The same C++ that powers bin/blackjack_server is
# compiled to WebAssembly and runs in the visitor's browser, so there is no
# backend to host, no cold start, and no shared state between visitors.
#
# Requires the Emscripten SDK:
#   git clone https://github.com/emscripten-core/emsdk && cd emsdk
#   ./emsdk install latest && ./emsdk activate latest
#   source ./emsdk_env.sh
#
# Usage: ./build-wasm.sh
set -euo pipefail

cd "$(dirname "$0")"

if ! command -v em++ >/dev/null 2>&1; then
  echo "em++ not found. Run:  source /path/to/emsdk/emsdk_env.sh" >&2
  exit 1
fi

OUT=docs
mkdir -p "$OUT"

# The trained Q-tables are embedded into the module, so loadQTable("data/...")
# finds them in Emscripten's in-memory filesystem exactly as it does on disk.
# They are ~32K total, which is cheaper than a second round trip.
# em++ rather than emcc: the sources are C++, and emcc links against the C
# runtime only, which fails on operator new/delete.
em++ \
  -std=c++17 -O3 \
  -Iinclude \
  src/wasm/main.cpp \
  src/core/*.cpp \
  src/ai/*.cpp \
  src/api/*.cpp \
  --embed-file data@/data \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=BlackjackModule \
  -s EXPORTED_FUNCTIONS='["_api_init","_api_request","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=32MB \
  -s ENVIRONMENT=web \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  -o "$OUT/blackjack.js"

# The page itself is the same source the local server serves.
cp web/style.css web/app.js "$OUT/"

# Inject the module scripts ahead of app.js. Doing it here rather than shipping
# them in web/index.html keeps the dev build free of 404s for files that only
# exist in the static build.
python3 - "$OUT" <<'PY'
import re, sys, pathlib
out = pathlib.Path(sys.argv[1])
html = pathlib.Path('web/index.html').read_text()
inject = (
    '<script>window.BLACKJACK_WASM = true;</script>\n'
    '<script src="blackjack.js"></script>'
)
html, n = re.subn(r'<!-- wasm-scripts:.*?-->', inject, html, count=1, flags=re.S)
if n != 1:
    sys.exit('could not find the wasm-scripts marker in web/index.html')
(out / 'index.html').write_text(html)
PY

# GitHub Pages runs Jekyll by default, which ignores files it doesn't recognise.
touch "$OUT/.nojekyll"

echo
echo "Built $OUT/"
ls -lh "$OUT" | tail -n +2 | awk '{printf "  %-20s %s\n", $9, $5}'
echo
echo "Try it:   python3 -m http.server -d $OUT 8000"
echo "Publish:  commit docs/, then Settings -> Pages -> Source: main /docs"
