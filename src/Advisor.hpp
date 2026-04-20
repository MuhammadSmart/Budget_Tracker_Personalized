#pragma once
#include <string>
#include <vector>

class Profile;
class Ledger;

struct Insight {
    enum Kind { Pace, Forecast, Overspend, Rebalance, Praise, Driver };
    Kind kind;
    std::string category;   // empty for global
    std::string message;
    int severity = 0;       // 0 info, 1 caution, 2 warning
};

// Short tag rendered next to each category bar.
struct PaceTag {
    std::string glyph;  // "on pace", "under", "fast", "Nd left", "OVER"
    int severity = 0;
};

class Advisor {
public:
    std::vector<Insight> analyze(const Profile&, const Ledger&) const;
    PaceTag paceTag(const Profile&, const Ledger&, const std::string& category) const;

private:
    double historicalBaselineAt(const Ledger&, const std::string& cat, double progress) const;
};
