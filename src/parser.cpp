#include "satforge/parser.hpp"

#include <fstream>
#include <sstream>
#include <cctype>

namespace satforge {

CnfFormula parseDimacs(const std::string& text) {
    CnfFormula f;
    std::istringstream in(text);
    std::string line;
    bool sawHeader = false;

    while (std::getline(in, line)) {
        // trim leading spaces
        size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
        if (i >= line.size()) continue;
        if (line[i] == 'c') continue;

        if (line[i] == 'p') {
            std::istringstream hs(line.substr(i));
            std::string p, cnf;
            int nVars = 0, nClauses = 0;
            hs >> p >> cnf >> nVars >> nClauses;
            if (p != "p" || cnf != "cnf" || nVars < 0) {
                throw std::runtime_error("invalid DIMACS header: " + line);
            }
            f.nVars = nVars;
            f.clauses.reserve(static_cast<size_t>(std::max(0, nClauses)));
            sawHeader = true;
            continue;
        }

        std::istringstream ls(line.substr(i));
        std::vector<Lit> clause;
        int lit = 0;
        while (ls >> lit) {
            if (lit == 0) break;
            clause.push_back(lit);
            int v = lit < 0 ? -lit : lit;
            if (v > f.nVars) f.nVars = v;
        }
        if (!clause.empty()) f.clauses.push_back(std::move(clause));
    }

    if (!sawHeader && f.nVars == 0 && !f.clauses.empty()) {
        // infer vars from literals if header missing
        for (const auto& c : f.clauses)
            for (Lit l : c) {
                int v = l < 0 ? -l : l;
                if (v > f.nVars) f.nVars = v;
            }
    }
    return f;
}

CnfFormula parseDimacsFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open file: " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return parseDimacs(ss.str());
}

}  // namespace satforge
