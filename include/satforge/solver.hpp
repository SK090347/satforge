#pragma once

#include "satforge/types.hpp"
#include "satforge/parser.hpp"
#include <vector>
#include <deque>
#include <string>

namespace satforge {

struct SolverStats {
    uint64_t decisions{0};
    uint64_t propagations{0};
    uint64_t conflicts{0};
    uint64_t learnt{0};
};

/// Educational CDCL SAT solver: unit prop, 1-UIP conflict analysis,
/// non-chronological backjump, VSIDS-style activity heuristic.
class Solver {
public:
    explicit Solver(const CnfFormula& formula);

    Status solve();
    const std::vector<int>& model() const { return model_; }
    const SolverStats& stats() const { return stats_; }

    /// Human-readable result line (SATISFIABLE / UNSATISFIABLE + model).
    std::string resultString() const;

private:
    int nVars_{0};
    std::vector<Clause> clauses_;
    std::vector<int8_t> assigns_;      // -1 undef, 0 false, 1 true
    std::vector<int> level_;
    std::vector<int> reason_;          // clause id or -1 for decision
    std::vector<Lit> trail_;
    std::vector<int> trail_lim_;
    std::deque<Lit> queue_;
    std::vector<double> activity_;
    std::vector<std::vector<ClauseId>> watches_;  // indexed by litToIndex
    double var_inc_{1.0};
    double clause_inc_{1.0};
    int decision_level_{0};
    Status status_{Status::Unknown};
    std::vector<int> model_;
    SolverStats stats_;

    int litToIndex(Lit l) const { return (var(l) << 1) | (sign(l) ? 1 : 0); }
    bool value(Lit l) const;
    void assign(Lit l, int reason_cid);
    bool enqueue(Lit l, int reason_cid);
    bool propagate(ClauseId& conflict);
    void undoOne();
    void cancelUntil(int level);
    Lit pickBranch();
    void bumpVar(Var v);
    void bumpClause(ClauseId cid);
    void decayActivities();
    bool analyze(ClauseId conflict, std::vector<Lit>& learnt, int& backtrack);
    void attachClause(ClauseId cid);
    ClauseId addLearnt(const std::vector<Lit>& lits);
};

}  // namespace satforge
