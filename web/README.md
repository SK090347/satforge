# satforge web playground

In-browser TypeScript **DPLL** for small CNF demos.

The production solver is the C++17 **CDCL** binary. Compiling that core to WASM with
[Emscripten](https://emscripten.org/) is optional and not required for CI:

```bash
# optional, if emcc is installed
emcc -O2 -std=c++17 \
  ../src/parser.cpp ../src/solver.cpp wasm_shim.cpp \
  -I../include -o public/satforge.js \
  -s MODULARIZE=1 -s EXPORT_NAME=createSatforge
```

Without emscripten, open `index.html` after `npm run build` (serves `dist/*.js`).
