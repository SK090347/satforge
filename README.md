# satforge


**Live demo:** https://sk090347.github.io/satforge/
Educational CDCL SAT solver. The serious engine is C++17 (watched literals, 1-UIP learning, VSIDS-ish branching); Python drives fixtures via the CLI; there’s a tiny TypeScript DPLL playground for demos in the browser.

[![CI](https://github.com/SK090347/satforge/actions/workflows/ci.yml/badge.svg)](https://github.com/SK090347/satforge/actions/workflows/ci.yml)
[![License: MIT OR Apache-2.0](https://img.shields.io/badge/license-MIT%20OR%20Apache--2.0-blue.svg)](LICENSE)

Author: Sumit Kumar Ta (SK090347)

## Notes

CNF:

\[
\phi = \bigwedge_{c \in C} \bigvee_{\ell \in c} \ell
\]

**Unit propagation.** A clause with one unset literal left forces it. Two-watched literals find units without scanning everything.

**1-UIP learning.** Resolve back along the implication graph until a unique implication point on the current decision level; the learnt clause is asserting after backjump. Search effort tracks conflicts; worst-case SAT stays NP-complete.

| Quantity | Role |
|----------|------|
| Decisions / conflicts | Search counters |
| Learnt clauses | Compact conflict summaries |
| Activity (VSIDS-ish) | Branch heuristic |
| Model / UNSAT | Assignment or proof of none |

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

./build/satforge examples/sat_simple.cnf    # exit 10 = SAT
./build/satforge examples/unsat_simple.cnf  # exit 20 = UNSAT

make test
# or: ctest --test-dir build --output-on-failure
SATFORGE_BIN=$PWD/build/satforge python3 -m pytest python/tests -q
```

Web playground (TS DPLL only — flagship remains C++):

```bash
cd web && npm install && npm run build
# open index.html (loads dist/main.js)
```

Optional WASM notes: [`web/README.md`](web/README.md).

## Layout

```
include/satforge/   types, parser, solver API
src/                parser.cpp, solver.cpp, main.cpp
python/             CLI wrapper + pytest
web/src/            TypeScript DPLL + UI
examples/           DIMACS fixtures
```

| Piece | Role |
|-------|------|
| Watched literals | Lazy unit detection |
| Trail + levels | Assignment stack for undo |
| 1-UIP learning | Asserting clause from conflict |
| Backjump | Undo to second-highest level in learnt clause |
| Activity | Bump on conflict; decay; pick max undef |

## Example

```text
p cnf 2 2
1 2 0
-1 2 0
```

```bash
$ ./build/satforge examples/sat_simple.cnf --stats
s SATISFIABLE
v -1 2 0
```

## License

**MIT** and **Apache-2.0** — see LICENSE files and NOTICE.
