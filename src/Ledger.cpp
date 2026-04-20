#include "Ledger.hpp"
#include "Profile.hpp"
#include "util/Date.hpp"
#include <algorithm>

double Ledger::spent(const std::string& cat) const {
    double s = 0;
    for (auto& t : current) if (t.category == cat) s += t.amount;
    return s;
}
double Ledger::spentItem(const std::string& cat, const std::string& item) const {
    double s = 0;
    for (auto& t : current)
        if (t.category == cat && t.item == item) s += t.amount;
    return s;
}
double Ledger::spentTotal() const {
    double s = 0;
    for (auto& t : current) s += t.amount;
    return s;
}
std::map<std::string,double> Ledger::itemBreakdown(const std::string& cat) const {
    std::map<std::string,double> m;
    for (auto& t : current)
        if (t.category == cat) m[t.item.empty() ? "(other)" : t.item] += t.amount;
    return m;
}

double Ledger::daysElapsed() const {
    return std::max(0.0, dates::daysBetween(cycleStart, dates::now()));
}
double Ledger::daysRemaining(const Profile& p) const {
    double len = double(p.cycleLengthSec()) / dates::DAY;
    return std::max(0.0, len - daysElapsed());
}
double Ledger::cycleProgress(const Profile& p) const {
    double len = double(p.cycleLengthSec()) / dates::DAY;
    if (len <= 0) return 0;
    return std::min(1.0, daysElapsed() / len);
}

static CycleSnapshot makeSnapshot(const Profile& p,
                                  const std::vector<Transaction>& txs,
                                  std::time_t start, std::time_t end) {
    CycleSnapshot s;
    s.start = start; s.end = end;
    for (auto& c : p.categories) {
        s.budget[c.name] = c.budget;
        s.spent[c.name] = 0.0;
    }
    for (auto& t : txs) s.spent[t.category] += t.amount;
    return s;
}

int Ledger::rolloverIfNeeded(Profile& p) {
    if (cycleStart == 0) cycleStart = p.cycleStart;
    long len = p.cycleLengthSec();
    std::time_t now = dates::now();
    int rolled = 0;
    while (cycleStart + len <= now) {
        std::time_t end = cycleStart + len;
        // partition transactions belonging to [cycleStart, end)
        std::vector<Transaction> closing, remain;
        for (auto& t : current) (t.ts < end ? closing : remain).push_back(t);
        history.push_back(makeSnapshot(p, closing, cycleStart, end));
        current = std::move(remain);
        cycleStart = end;
        ++rolled;
        if (rolled > 120) break; // safety: ~10y of weekly cycles
    }
    p.cycleStart = cycleStart;
    return rolled;
}

void Ledger::forceRollover(Profile& p) {
    std::time_t now = dates::now();
    history.push_back(makeSnapshot(p, current, cycleStart, now));
    current.clear();
    cycleStart = now;
    p.cycleStart = cycleStart;
}
