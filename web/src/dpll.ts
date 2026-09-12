/**
 * Tiny educational DPLL SAT solver for the browser playground.
 * The production engine is the C++17 CDCL binary (`satforge`); this port
 * exists so the HTML UI works without WASM/emscripten.
 */

export type Lit = number;
export type Clause = Lit[];
export type Formula = Clause[];

export interface SolveResult {
  status: "SAT" | "UNSAT";
  model?: number[];
  decisions: number;
  propagations: number;
}

function parseDimacs(text: string): { nVars: number; clauses: Formula } {
  let nVars = 0;
  const clauses: Formula = [];
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (!line || line.startsWith("c")) continue;
    if (line.startsWith("p")) {
      const parts = line.split(/\s+/);
      nVars = parseInt(parts[2], 10) || 0;
      continue;
    }
    const nums = line.split(/\s+/).map(Number).filter((x) => !Number.isNaN(x));
    const lits: Lit[] = [];
    for (const n of nums) {
      if (n === 0) break;
      lits.push(n);
      nVars = Math.max(nVars, Math.abs(n));
    }
    if (lits.length) clauses.push(lits);
  }
  return { nVars, clauses };
}

function simplify(clauses: Formula, assign: Map<number, boolean>): Formula | null {
  const out: Formula = [];
  for (const clause of clauses) {
    let sat = false;
    const kept: Lit[] = [];
    for (const lit of clause) {
      const v = Math.abs(lit);
      if (!assign.has(v)) {
        kept.push(lit);
        continue;
      }
      const val = assign.get(v)!;
      const litTrue = lit > 0 ? val : !val;
      if (litTrue) {
        sat = true;
        break;
      }
    }
    if (sat) continue;
    if (kept.length === 0) return null; // conflict
    out.push(kept);
  }
  return out;
}

function unitPropagate(
  clauses: Formula,
  assign: Map<number, boolean>,
  stats: { propagations: number }
): Formula | null {
  let cur: Formula | null = clauses;
  let changed = true;
  while (changed && cur) {
    changed = false;
    for (const c of cur) {
      if (c.length !== 1) continue;
      const lit = c[0];
      const v = Math.abs(lit);
      const want = lit > 0;
      if (assign.has(v) && assign.get(v) !== want) return null;
      if (!assign.has(v)) {
        assign.set(v, want);
        stats.propagations++;
        changed = true;
      }
    }
    if (changed) cur = simplify(cur, assign);
  }
  return cur;
}

function pickVar(clauses: Formula, assign: Map<number, boolean>): number | null {
  for (const c of clauses) {
    for (const lit of c) {
      const v = Math.abs(lit);
      if (!assign.has(v)) return v;
    }
  }
  return null;
}

function dpll(
  clauses: Formula,
  assign: Map<number, boolean>,
  stats: { decisions: number; propagations: number }
): boolean {
  const simplified = unitPropagate(clauses, assign, stats);
  if (simplified === null) return false;
  if (simplified.length === 0) return true;
  const v = pickVar(simplified, assign);
  if (v === null) return true;
  for (const val of [true, false]) {
    stats.decisions++;
    const next = new Map(assign);
    next.set(v, val);
    const nextClauses = simplify(simplified, next);
    if (nextClauses === null) continue;
    if (dpll(nextClauses, next, stats)) {
      assign.clear();
      for (const [k, vv] of next) assign.set(k, vv);
      return true;
    }
  }
  return false;
}

export function solveCnf(text: string): SolveResult {
  const { nVars, clauses } = parseDimacs(text);
  if (clauses.some((c) => c.length === 0)) {
    return { status: "UNSAT", decisions: 0, propagations: 0 };
  }
  const assign = new Map<number, boolean>();
  const stats = { decisions: 0, propagations: 0 };
  const ok = dpll(clauses, assign, stats);
  if (!ok) return { status: "UNSAT", ...stats };
  const model: number[] = [];
  for (let v = 1; v <= nVars; v++) {
    const val = assign.has(v) ? assign.get(v)! : false;
    model.push(val ? v : -v);
  }
  return { status: "SAT", model, ...stats };
}

export const DEMOS: Record<string, string> = {
  sat_simple: `c Satisfiable: (x1 ∨ x2) ∧ (¬x1 ∨ x2)
p cnf 2 2
1 2 0
-1 2 0`,
  unsat_simple: `c Unsatisfiable: x1 ∧ ¬x1
p cnf 1 2
1 0
-1 0`,
  sat_3: `c Small 3-SAT (SAT)
p cnf 4 5
1 2 3 0
-1 -2 0
-3 4 0
2 -4 0
-1 3 4 0`,
  unsat_chain: `c Forced contradiction (UNSAT)
p cnf 2 4
1 2 0
-1 2 0
1 -2 0
-1 -2 0`,
};
