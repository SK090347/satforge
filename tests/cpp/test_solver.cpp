#include "satforge/parser.hpp"
#include "satforge/solver.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace satforge;

static void expect_sat(const std::string& dimacs) {
    auto f = parseDimacs(dimacs);
    Solver s(f);
    assert(s.solve() == Status::Sat);
}

static void expect_unsat(const std::string& dimacs) {
    auto f = parseDimacs(dimacs);
    Solver s(f);
    assert(s.solve() == Status::Unsat);
}

int main() {
    expect_sat(
        "p cnf 1 1\n"
        "1 0\n");

    expect_sat(
        "c trivial sat\n"
        "p cnf 2 2\n"
        "1 2 0\n"
        "-1 2 0\n");

    expect_unsat(
        "p cnf 1 2\n"
        "1 0\n"
        "-1 0\n");

    expect_unsat(
        "p cnf 3 4\n"
        "1 2 0\n"
        "-1 2 0\n"
        "1 -2 0\n"
        "-1 -2 0\n");

    // pigeonhole PHP(2,1) style tiny unsat
    expect_unsat(
        "p cnf 2 3\n"
        "1 2 0\n"
        "-1 0\n"
        "-2 0\n");

    // 3-SAT sat
    expect_sat(
        "p cnf 3 3\n"
        "1 2 3 0\n"
        "-1 -2 0\n"
        "-3 2 0\n");

    std::cout << "ok — all C++ solver tests passed\n";
    return 0;
}
