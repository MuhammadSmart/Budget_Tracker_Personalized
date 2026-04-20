#include "Storage.hpp"
#include "Profile.hpp"
#include "Ledger.hpp"
#include "util/Text.hpp"
#include <fstream>
#include <sys/stat.h>

Storage::Storage(std::string dir) : dir_(std::move(dir)) {
#ifdef _WIN32
    _mkdir(dir_.c_str());
#else
    mkdir(dir_.c_str(), 0755);
#endif
}

std::string Storage::profilePath() const { return dir_ + "/profile.cfg"; }
std::string Storage::ledgerPath()  const { return dir_ + "/ledger.log"; }
std::string Storage::historyPath() const { return dir_ + "/history.tsv"; }

bool Storage::profileExists() const {
    std::ifstream f(profilePath());
    return f.good();
}

// ---------------- profile ----------------

void Storage::saveProfile(const Profile& p) const {
    std::ofstream f(profilePath());
    f << "# Budget Tracker profile\n";
    f << "period      = " << (p.period == Period::Weekly ? "weekly" : "monthly") << "\n";
    f << "currency    = " << p.currency << "\n";
    f << "cycle_start = " << p.cycleStart << "\n\n";
    for (auto& c : p.categories) {
        f << "[" << c.name << "]\n";
        f << "budget = " << c.budget << "\n";
        f << "items  = ";
        for (size_t i = 0; i < c.items.size(); ++i) {
            if (i) f << ", ";
            f << c.items[i].name;
            for (auto& a : c.items[i].aliases) f << "|" << a;
        }
        f << "\n\n";
    }
}

Profile Storage::loadProfile() const {
    Profile p;
    std::ifstream f(profilePath());
    std::string line;
    Category* cur = nullptr;
    while (std::getline(f, line)) {
        line = text::trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            Category c;
            c.name = text::lower(line.substr(1, line.size() - 2));
            p.categories.push_back(std::move(c));
            cur = &p.categories.back();
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = text::lower(text::trim(line.substr(0, eq)));
        std::string val = text::trim(line.substr(eq + 1));
        if (!cur) {
            if (key == "period")
                p.period = (text::lower(val).rfind("w",0)==0) ? Period::Weekly : Period::Monthly;
            else if (key == "currency") p.currency = val;
            else if (key == "cycle_start") p.cycleStart = (std::time_t)std::stoll(val);
        } else {
            if (key == "budget") text::parseDouble(val, cur->budget);
            else if (key == "items") {
                for (auto& raw : text::split(val, ',')) {
                    if (raw.empty()) continue;
                    auto parts = text::split(raw, '|');
                    Item it; it.name = text::lower(parts[0]);
                    for (size_t i = 1; i < parts.size(); ++i)
                        it.aliases.push_back(text::lower(parts[i]));
                    cur->items.push_back(std::move(it));
                }
            }
        }
    }
    return p;
}

// ---------------- ledger ----------------

void Storage::appendTransaction(const Transaction& t) const {
    std::ofstream f(ledgerPath(), std::ios::app);
    f << t.ts << "\t" << t.category << "\t"
      << (t.item.empty() ? "-" : t.item) << "\t"
      << t.amount << "\t" << t.note << "\n";
}

void Storage::saveLedger(const Ledger& l) const {
    {
        std::ofstream f(ledgerPath());
        f << "# ts\tcategory\titem\tamount\tnote\n";
        for (auto& t : l.current)
            f << t.ts << "\t" << t.category << "\t"
              << (t.item.empty() ? "-" : t.item) << "\t"
              << t.amount << "\t" << t.note << "\n";
    }
    {
        std::ofstream f(historyPath());
        f << "# start\tend\tcategory\tbudget\tspent\n";
        for (auto& s : l.history)
            for (auto& [cat, sp] : s.spent) {
                double b = 0;
                auto it = s.budget.find(cat);
                if (it != s.budget.end()) b = it->second;
                f << s.start << "\t" << s.end << "\t" << cat << "\t"
                  << b << "\t" << sp << "\n";
            }
    }
}

Ledger Storage::loadLedger(const Profile& p) const {
    Ledger l;
    l.cycleStart = p.cycleStart;
    {
        std::ifstream f(ledgerPath());
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto parts = text::split(line, '\t');
            if (parts.size() < 4) continue;
            Transaction t;
            t.ts = (std::time_t)std::stoll(parts[0]);
            t.category = parts[1];
            t.item = (parts[2] == "-") ? "" : parts[2];
            text::parseDouble(parts[3], t.amount);
            if (parts.size() > 4) t.note = parts[4];
            l.current.push_back(std::move(t));
        }
    }
    {
        std::ifstream f(historyPath());
        std::string line;
        std::map<std::pair<std::time_t,std::time_t>, CycleSnapshot> grouped;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto parts = text::split(line, '\t');
            if (parts.size() < 5) continue;
            std::time_t s = (std::time_t)std::stoll(parts[0]);
            std::time_t e = (std::time_t)std::stoll(parts[1]);
            auto& snap = grouped[{s,e}];
            snap.start = s; snap.end = e;
            double b=0, sp=0;
            text::parseDouble(parts[3], b);
            text::parseDouble(parts[4], sp);
            snap.budget[parts[2]] = b;
            snap.spent[parts[2]] = sp;
        }
        for (auto& [k,v] : grouped) l.history.push_back(v);
    }
    return l;
}
