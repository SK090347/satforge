#include "satforge/parser.hpp"
#include "satforge/solver.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static void usage(const char* argv0) {
    std::cerr << "satforge — educational CDCL SAT solver\n"
              << "Usage: " << argv0 << " [options] [file.cnf]\n"
              << "  -h, --help     show help\n"
              << "  --stats        print decision/conflict stats to stderr\n"
              << "  (no file)      read DIMACS CNF from stdin\n";
}

int main(int argc, char** argv) {
    bool showStats = false;
    std::string path;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            usage(argv[0]);
            return 0;
        }
        if (a == "--stats") {
            showStats = true;
            continue;
        }
        if (a[0] == '-') {
            std::cerr << "unknown option: " << a << "\n";
            usage(argv[0]);
            return 2;
        }
        path = a;
    }

    try {
        satforge::CnfFormula formula;
        if (path.empty()) {
            std::ostringstream ss;
            ss << std::cin.rdbuf();
            formula = satforge::parseDimacs(ss.str());
        } else {
            formula = satforge::parseDimacsFile(path);
        }

        satforge::Solver solver(formula);
        auto st = solver.solve();
        std::cout << solver.resultString();
        if (showStats) {
            const auto& s = solver.stats();
            std::cerr << "c decisions=" << s.decisions
                      << " propagations=" << s.propagations
                      << " conflicts=" << s.conflicts
                      << " learnt=" << s.learnt << "\n";
        }
        return st == satforge::Status::Sat ? 10
             : st == satforge::Status::Unsat ? 20
             : 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
