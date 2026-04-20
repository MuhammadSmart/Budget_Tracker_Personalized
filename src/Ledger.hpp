#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>

class Profile;

struct Transaction {
    std::time_t ts = 0;
    std::string category;
    std::string item;     // may be empty
    double amount = 0.0;
    std::string note;
};

struct CycleSnapshot {
    std::time_t start = 0, end = 0;
    std::map<std::string, double> spent;   // per category
    std::map<std::string, double> budget;  // per category (budgets can change)
};

class Ledger {
public:
    std::time_t cycleStart = 0;
    std::vector<Transaction> current;
    std::vector<CycleSnapshot> history;

    void log(const Transaction& t) { current.push_back(t); }

    double spent(const std::string& category) const;
    double spentItem(const std::string& category, const std::string& item) const;
    double spentTotal() const;
    std::map<std::string, double> itemBreakdown(const std::string& category) const;

    // 0..1 — how far through the current cycle we are right now.
    double cycleProgress(const Profile& p) const;
    double daysElapsed() const;
    double daysRemaining(const Profile& p) const;

    // If today is past cycle end, archive current → history (possibly multiple
    // cycles if the app wasn't opened for a while) and reset cycleStart.
    // Returns number of cycles rolled over.
    int rolloverIfNeeded(Profile& p);
    void forceRollover(Profile& p);
};
