#pragma once

#include "satforge/types.hpp"
#include <string>
#include <vector>
#include <stdexcept>

namespace satforge {

struct CnfFormula {
    int nVars{0};
    std::vector<std::vector<Lit>> clauses;
};

/// Parse DIMACS CNF from a string (supports comments and 'c'/'p' lines).
CnfFormula parseDimacs(const std::string& text);

/// Parse DIMACS CNF from a file path.
CnfFormula parseDimacsFile(const std::string& path);

}  // namespace satforge
