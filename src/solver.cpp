#include "satforge/solver.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace satforge {

Solver::Solver(const CnfFormula& formula) : nVars_(formula.nVars) {
    assigns_.assign(static_cast<size_t>(nVars_) + 1, -1);
    level_.assign(static_cast<size_t>(nVars_) + 1, -1);
    reason_.assign(static_cast<size_t>(nVars_) + 1, -1);
    activity_.assign(static_cast<size_t>(nVars_) + 1, 0.0);
    watches_.assign(static_cast<size_t>((nVars_ + 1) * 2), {});

    for (const auto& lits : formula.clauses) {
        if (lits.empty()) {
            status_ = Status::Unsat;
            continue;
        }
        Clause c;
        c.lits = lits;
        // remove tautologies / duplicates lightly
        std::sort(c.lits.begin(), c.lits.end(),
                  [](Lit a, Lit b) { return var(a) < var(b) || (var(a) == var(b) && a < b); });
        c.lits.erase(std::unique(c.lits.begin(), c.lits.end()), c.lits.end());
        bool taut = false;
        for (size_t i = 1; i < c.lits.size(); ++i) {
            if (c.lits[i] == -c.lits[i - 1]) { taut = true; break; }
        }
        if (taut) continue;
        clauses_.push_back(std::move(c));
        attachClause(static_cast<ClauseId>(clauses_.size() - 1));
    }
}

bool Solver::value(Lit l) const {
    int8_t a = assigns_[static_cast<size_t>(var(l))];
    if (a < 0) return false;  // caller must check undef separately
    bool isTrue = (a == 1);
    return sign(l) ? !isTrue : isTrue;
}

void Solver::attachClause(ClauseId cid) {
    auto& c = clauses_[static_cast<size_t>(cid)];
    if (c.lits.size() == 1) {
        // unit — enqueue later in solve
        return;
    }
    watches_[static_cast<size_t>(litToIndex(c.lits[0]))].push_back(cid);
    watches_[static_cast<size_t>(litToIndex(c.lits[1]))].push_back(cid);
}

bool Solver::enqueue(Lit l, int reason_cid) {
    Var v = var(l);
    int8_t& a = assigns_[static_cast<size_t>(v)];
    if (a >= 0) {
        bool wantTrue = !sign(l);
        return (a == 1) == wantTrue;
    }
    a = sign(l) ? 0 : 1;
    level_[static_cast<size_t>(v)] = decision_level_;
    reason_[static_cast<size_t>(v)] = reason_cid;
    trail_.push_back(l);
    queue_.push_back(l);
    ++stats_.propagations;
    return true;
}

void Solver::assign(Lit l, int reason_cid) {
    if (!enqueue(l, reason_cid)) {
        throw std::runtime_error("conflicting assign");
    }
}

bool Solver::propagate(ClauseId& conflict) {
    while (!queue_.empty()) {
        Lit p = queue_.front();
        queue_.pop_front();
        auto& ws = watches_[static_cast<size_t>(litToIndex(neg(p)))];
        size_t i = 0;
        while (i < ws.size()) {
            ClauseId cid = ws[i];
            Clause& c = clauses_[static_cast<size_t>(cid)];
            // Ensure watched lit that is ~p is at index 1
            if (c.lits[0] == neg(p)) std::swap(c.lits[0], c.lits[1]);
            Lit first = c.lits[0];
            // If first is already true, clause satisfied
            Var fv = var(first);
            int8_t fa = assigns_[static_cast<size_t>(fv)];
            if (fa >= 0) {
                bool firstTrue = sign(first) ? (fa == 0) : (fa == 1);
                if (firstTrue) {
                    ++i;
                    continue;
                }
            }
            // Look for another watch
            bool found = false;
            for (size_t k = 2; k < c.lits.size(); ++k) {
                Lit lit = c.lits[k];
                Var lv = var(lit);
                int8_t la = assigns_[static_cast<size_t>(lv)];
                if (la < 0 || (sign(lit) ? (la == 0) : (la == 1))) {
                    // move lit to watch position 1
                    c.lits[1] = lit;
                    c.lits[k] = neg(p);
                    watches_[static_cast<size_t>(litToIndex(lit))].push_back(cid);
                    ws[i] = ws.back();
                    ws.pop_back();
                    found = true;
                    break;
                }
            }
            if (found) continue;

            // No replacement — unit or conflict
            ++i;
            Var v0 = var(first);
            int8_t a0 = assigns_[static_cast<size_t>(v0)];
            if (a0 < 0) {
                if (!enqueue(first, cid)) {
                    conflict = cid;
                    return false;
                }
            } else {
                bool firstTrue = sign(first) ? (a0 == 0) : (a0 == 1);
                if (!firstTrue) {
                    conflict = cid;
                    return false;
                }
            }
        }
    }
    return true;
}

void Solver::undoOne() {
    Lit l = trail_.back();
    trail_.pop_back();
    Var v = var(l);
    assigns_[static_cast<size_t>(v)] = -1;
    reason_[static_cast<size_t>(v)] = -1;
    level_[static_cast<size_t>(v)] = -1;
}

void Solver::cancelUntil(int level) {
    while (decision_level_ > level) {
        while (static_cast<int>(trail_.size()) > trail_lim_.back()) undoOne();
        trail_lim_.pop_back();
        --decision_level_;
    }
    queue_.clear();
}

void Solver::bumpVar(Var v) {
    activity_[static_cast<size_t>(v)] += var_inc_;
    if (activity_[static_cast<size_t>(v)] > 1e100) {
        for (int i = 1; i <= nVars_; ++i) activity_[static_cast<size_t>(i)] *= 1e-100;
        var_inc_ *= 1e-100;
    }
}

void Solver::bumpClause(ClauseId cid) {
    clauses_[static_cast<size_t>(cid)].activity += clause_inc_;
}

void Solver::decayActivities() {
    var_inc_ *= (1.0 / 0.95);
    clause_inc_ *= (1.0 / 0.999);
}

Lit Solver::pickBranch() {
    Var best = 0;
    double bestAct = -1.0;
    for (Var v = 1; v <= nVars_; ++v) {
        if (assigns_[static_cast<size_t>(v)] < 0 && activity_[static_cast<size_t>(v)] > bestAct) {
            bestAct = activity_[static_cast<size_t>(v)];
            best = v;
        }
    }
    if (best == 0) return 0;
    return mkLit(best, false);  // positive polarity bias
}

bool Solver::analyze(ClauseId conflict, std::vector<Lit>& learnt, int& backtrack) {
    learnt.clear();
    learnt.push_back(0);  // placeholder for asserting literal
    int pathC = 0;
    Lit p = 0;
    std::vector<char> seen(static_cast<size_t>(nVars_) + 1, 0);
    int index = static_cast<int>(trail_.size()) - 1;

    do {
        if (conflict < 0) return false;
        Clause& c = clauses_[static_cast<size_t>(conflict)];
        bumpClause(conflict);
        for (size_t j = (p == 0 ? 0 : 1); j < c.lits.size(); ++j) {
            Lit q = c.lits[j];
            Var v = var(q);
            if (!seen[static_cast<size_t>(v)] && level_[static_cast<size_t>(v)] > 0) {
                seen[static_cast<size_t>(v)] = 1;
                bumpVar(v);
                if (level_[static_cast<size_t>(v)] >= decision_level_) {
                    ++pathC;
                } else {
                    learnt.push_back(q);
                }
            }
        }
        while (!seen[static_cast<size_t>(var(trail_[static_cast<size_t>(index)]))]) --index;
        p = trail_[static_cast<size_t>(index)];
        --index;
        conflict = reason_[static_cast<size_t>(var(p))];
        seen[static_cast<size_t>(var(p))] = 0;
        --pathC;
    } while (pathC > 0);

    learnt[0] = neg(p);

    // UIP learnt clause — compute backjump level
    if (learnt.size() == 1) {
        backtrack = 0;
    } else {
        int max_i = 1;
        for (size_t i = 2; i < learnt.size(); ++i) {
            if (level_[static_cast<size_t>(var(learnt[i]))] >
                level_[static_cast<size_t>(var(learnt[max_i]))]) {
                max_i = static_cast<int>(i);
            }
        }
        std::swap(learnt[1], learnt[static_cast<size_t>(max_i)]);
        backtrack = level_[static_cast<size_t>(var(learnt[1]))];
    }
    return true;
}

ClauseId Solver::addLearnt(const std::vector<Lit>& lits) {
    Clause c;
    c.lits = lits;
    c.learnt = true;
    c.activity = 0.0;
    clauses_.push_back(std::move(c));
    ClauseId cid = static_cast<ClauseId>(clauses_.size() - 1);
    if (lits.size() >= 2) attachClause(cid);
    ++stats_.learnt;
    return cid;
}

Status Solver::solve() {
    if (status_ == Status::Unsat) return status_;

    // Enqueue initial units
    for (ClauseId cid = 0; cid < static_cast<ClauseId>(clauses_.size()); ++cid) {
        if (clauses_[static_cast<size_t>(cid)].lits.size() == 1) {
            if (!enqueue(clauses_[static_cast<size_t>(cid)].lits[0], cid)) {
                status_ = Status::Unsat;
                return status_;
            }
        }
    }

    ClauseId conflict = -1;
    for (;;) {
        if (!propagate(conflict)) {
            ++stats_.conflicts;
            if (decision_level_ == 0) {
                status_ = Status::Unsat;
                return status_;
            }
            std::vector<Lit> learnt;
            int bt = 0;
            if (!analyze(conflict, learnt, bt)) {
                status_ = Status::Unsat;
                return status_;
            }
            cancelUntil(bt);
            ClauseId cid = addLearnt(learnt);
            enqueue(learnt[0], learnt.size() == 1 ? -1 : cid);
            decayActivities();
            conflict = -1;
            continue;
        }

        Lit next = pickBranch();
        if (next == 0) {
            // all assigned
            model_.assign(static_cast<size_t>(nVars_) + 1, 0);
            for (Var v = 1; v <= nVars_; ++v) {
                model_[static_cast<size_t>(v)] = assigns_[static_cast<size_t>(v)] == 1 ? v : -v;
            }
            status_ = Status::Sat;
            return status_;
        }
        trail_lim_.push_back(static_cast<int>(trail_.size()));
        ++decision_level_;
        ++stats_.decisions;
        enqueue(next, -1);
    }
}

std::string Solver::resultString() const {
    std::ostringstream out;
    if (status_ == Status::Sat) {
        out << "s SATISFIABLE\n";
        out << "v";
        for (Var v = 1; v <= nVars_; ++v) out << " " << model_[static_cast<size_t>(v)];
        out << " 0\n";
    } else if (status_ == Status::Unsat) {
        out << "s UNSATISFIABLE\n";
    } else {
        out << "s UNKNOWN\n";
    }
    return out.str();
}

}  // namespace satforge
