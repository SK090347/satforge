# satforge

**Educational CDCL SAT solver** — MIT 6.034 / contest-solver caliber, portfolio-ready.

Parse DIMACS CNF, run **unit propagation**, **conflict analysis** with clause learning,
**non-chronological backjumping**, and a **VSIDS-style** activity heuristic — then wrap it
with Python tests and a tiny TypeScript playground.

> Author: **Sumit Kumar Ta (SK090347)** · Dual license **MIT OR Apache-2.0**

---

## Why this exists

Boolean satisfiability is the canonical NP-complete problem and the engine behind modern
verification, planning, and synthesis. Reading about CDCL is necessary;
**implementing** watched literals, 1-UIP learning, and activity-based branching is what
makes compilers / PL / algorithms interviews click.

`satforge` packages:

1. A **C++17 CDCL core** you can read like a teaching lab.
2. A **Python harness** (`pytest` via subprocess CLI) for sat/unsat fixtures.
3. A **TypeScript + HTML playground** with an in-browser DPLL for demos when WASM is unavailable.

---

## Quick start

```bash
# Build the CLI
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
# or: make build

./build/satforge examples/sat_simple.cnf
# s SATISFIABLE
# v -1 2 0

./build/satforge examples/unsat_simple.cnf
# s UNSATISFIABLE

# C++ + Python tests
make test
# or:
ctest --test-dir build --output-on-failure
SATFORGE_BIN=$PWD/build/satforge python3 -m pytest python/tests -q
```

Exit codes follow the SAT competition convention: **10** = SAT, **20** = UNSAT.

### Web playground

```bash
cd web && npm install && npm run build
# open index.html in a browser (module loads dist/main.js)
```

The browser uses a tiny **TypeScript DPLL**. The flagship engine remains the C++ CDCL binary.
Optional Emscripten WASM notes live in [`web/README.md`](web/README.md).

---

## Architecture

```mermaid
flowchart TB
  subgraph Input
    DIMACS[DIMACS CNF file / stdin]
  end

  subgraph Core["C++17 CDCL core"]
    Parse[Parser]
    UP[2-watched unit propagation]
    Decide[VSIDS-ish branch]
    CA[1-UIP conflict analysis]
    Learn[Clause learning + backjump]
  end

  subgraph Frontends
    CLI[satforge CLI]
    Py[Python subprocess harness]
    Web[TS DPLL playground]
  end

  DIMACS --> Parse --> UP
  UP -->|conflict| CA --> Learn --> UP
  UP -->|ok| Decide --> UP
  UP -->|complete assign| SAT[SAT + model]
  CA -->|level 0| UNSAT[UNSAT]
  CLI --> Core
  Py --> CLI
  Web -.->|demo only| TinyDPLL[In-browser DPLL]
```

### Core layout

```
include/satforge/   types, parser, solver API
src/                parser.cpp, solver.cpp, main.cpp
python/satforge/    CLI wrapper
python/tests/       pytest sat/unsat integration
web/src/            TypeScript DPLL + UI glue
examples/           tiny DIMACS fixtures
tests/cpp/          native smoke tests
```

### Algorithm sketch

| Piece | Role |
|-------|------|
| **Watched literals** | Lazy unit detection without scanning every clause |
| **Trail + levels** | Chronological assignment stack for undo |
| **1-UIP learning** | Derive asserting clause from conflict graph |
| **Backjump** | Undo to second-highest level in learnt clause |
| **Activity (VSIDS-ish)** | Bump vars on conflict; decay; pick max undef |

---

## DIMACS example

```text
c (x1 ∨ x2) ∧ (¬x1 ∨ x2)  →  forces x2
p cnf 2 2
1 2 0
-1 2 0
```

```bash
$ ./build/satforge examples/sat_simple.cnf --stats
s SATISFIABLE
v -1 2 0
c decisions=… propagations=… conflicts=… learnt=…
```

---

## Skills this demonstrates

- **Algorithms**: search, implication graphs, heuristics (NP-complete solvers)
- **Compilers / PL**: CNF as an IR; watched structures akin to pattern matching
- **Systems**: C++17, CMake, CLI contracts, CI matrix
- **Polyglot**: Python test harness + TypeScript educational UI

---

## License

Dual-licensed under **MIT** and **Apache-2.0**. See [`LICENSE`](LICENSE),
[`LICENSE-APACHE`](LICENSE-APACHE), and [`NOTICE`](NOTICE).

---

## Topics

`sat-solver` · `cdcl` · `cpp` · `python` · `typescript` · `algorithms` · `portfolio`
