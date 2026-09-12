#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace satforge {

using Lit = int;          // ±variable id (1-indexed); 0 is sentinel
using Var = int;          // 1..nVars
using ClauseId = int;

enum class Status { Unknown, Sat, Unsat };

struct Clause {
    std::vector<Lit> lits;
    bool learnt{false};
    double activity{0.0};
};

inline Lit mkLit(Var v, bool neg = false) { return neg ? -v : v; }
inline Var var(Lit l) { return l < 0 ? -l : l; }
inline bool sign(Lit l) { return l < 0; }
inline Lit neg(Lit l) { return -l; }

}  // namespace satforge
