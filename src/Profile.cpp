#include "Profile.hpp"
#include "util/Text.hpp"
#include "util/Date.hpp"
#include <iostream>
#include <limits>

double Profile::totalBudget() const {
    double t = 0;
    for (auto& c : categories) t += c.budget;
    return t;
}

Category* Profile::category(const std::string& name) {
    std::string n = text::lower(name);
    for (auto& c : categories) if (c.name == n) return &c;
    return nullptr;
}
const Category* Profile::category(const std::string& name) const {
    return const_cast<Profile*>(this)->category(name);
}

long Profile::cycleLengthSec() const {
    return dates::cycleLengthSec(period == Period::Weekly);
}

Profile::Match Profile::find(const std::string& token) const {
    std::string t = text::lower(token);
    if (t.empty()) return {};

    // 1. exact item name or alias
    for (auto& c : categories)
        for (auto& it : c.items) {
            if (it.name == t) return {&c, &it, true};
            for (auto& a : it.aliases) if (a == t) return {&c, &it, true};
        }
    // 2. exact category name
    for (auto& c : categories) if (c.name == t) return {&c, nullptr, true};

    // 3. prefix on item, then category
    for (auto& c : categories)
        for (auto& it : c.items)
            if (it.name.rfind(t, 0) == 0) return {&c, &it, false};
    for (auto& c : categories)
        if (c.name.rfind(t, 0) == 0) return {&c, nullptr, false};

    // 4. levenshtein <= 2 on items, then categories
    const Category* bestC = nullptr; const Item* bestI = nullptr; int bestD = 3;
    for (auto& c : categories) {
        for (auto& it : c.items) {
            int d = text::levenshtein(t, it.name);
            if (d < bestD) { bestD = d; bestC = &c; bestI = &it; }
        }
        int d = text::levenshtein(t, c.name);
        if (d < bestD) { bestD = d; bestC = &c; bestI = nullptr; }
    }
    if (bestC) return {bestC, bestI, false};
    return {};
}

std::string Profile::suggest(const std::string& token) const {
    std::string t = text::lower(token);
    std::string best; int bestD = std::numeric_limits<int>::max();
    auto consider = [&](const std::string& s){
        int d = text::levenshtein(t, s);
        if (d < bestD) { bestD = d; best = s; }
    };
    for (auto& c : categories) {
        consider(c.name);
        for (auto& it : c.items) consider(it.name);
    }
    return best;
}

// ---------------- onboarding ----------------

static std::string ask(std::istream& in, std::ostream& out,
                       const std::string& prompt, const std::string& def = "") {
    out << prompt;
    if (!def.empty()) out << " [" << def << "]";
    out << ": " << std::flush;
    std::string line;
    if (!std::getline(in, line)) return def;
    line = text::trim(line);
    return line.empty() ? def : line;
}

Profile Profile::fromOnboarding(std::istream& in, std::ostream& out) {
    Profile p;
    out << "\nNo profile found -- let's set one up.\n\n";

    std::string per = text::lower(ask(in, out, "Track weekly or monthly?", "monthly"));
    p.period = (per.rfind("w", 0) == 0) ? Period::Weekly : Period::Monthly;

    p.currency = ask(in, out, "Currency", "EGP");
    p.cycleStart = dates::now();

    out << "\nLet's add your spending categories.\n"
        << "Type:  <category name> <budget>   (blank line to finish)\n\n";

    while (true) {
        out << "category> " << std::flush;
        std::string line;
        if (!std::getline(in, line)) break;
        line = text::trim(line);
        if (line.empty()) break;

        auto toks = text::tokenize(line);
        double budget = 0;
        if (toks.size() < 2 || !text::parseDouble(toks.back(), budget)) {
            out << "  (format: name budget, e.g. 'groceries 800')\n";
            continue;
        }
        toks.pop_back();
        Category c;
        c.name = text::lower(text::join(toks, "_"));
        c.budget = budget;

        std::string itemsLine = ask(in, out,
            "  items in " + c.name + " (comma-separated, item|alias|alias)");
        for (auto& raw : text::split(itemsLine, ',')) {
            if (raw.empty()) continue;
            auto parts = text::split(raw, '|');
            Item it;
            it.name = text::lower(parts[0]);
            for (size_t i = 1; i < parts.size(); ++i)
                it.aliases.push_back(text::lower(parts[i]));
            c.items.push_back(std::move(it));
        }
        p.categories.push_back(std::move(c));
        out << "  added " << p.categories.back().name
            << " (" << p.categories.back().items.size() << " items)\n";
    }

    if (p.categories.empty()) {
        out << "\n(no categories entered -- creating a default 'misc' bucket)\n";
        p.categories.push_back({"misc", 100.0, {}});
    }
    out << "\nProfile created. Total "
        << (p.period == Period::Weekly ? "weekly" : "monthly")
        << " budget: " << text::money(p.totalBudget()) << " " << p.currency << "\n\n";
    return p;
}
