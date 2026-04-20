#include "Advisor.hpp"
#include "Profile.hpp"
#include "Ledger.hpp"
#include "util/Text.hpp"
#include "util/Date.hpp"
#include <algorithm>
#include <cmath>

// Weighted mean of historical end-of-cycle spend, scaled to current progress.
// Weights: most recent 3 cycles get 3,3,2; everything older gets 1.
double Advisor::historicalBaselineAt(const Ledger& l,
                                     const std::string& cat,
                                     double progress) const {
    if (l.history.size() < 2) return -1.0; // not enough data
    double wsum = 0, vsum = 0;
    size_t n = l.history.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& s = l.history[i];
        auto it = s.spent.find(cat);
        if (it == s.spent.end()) continue;
        size_t fromEnd = n - 1 - i; // 0 = most recent
        double w = (fromEnd == 0) ? 3.0 : (fromEnd == 1) ? 3.0
                 : (fromEnd == 2) ? 2.0 : 1.0;
        wsum += w;
        vsum += w * it->second;
    }
    if (wsum == 0) return -1.0;
    double meanFinal = vsum / wsum;
    return meanFinal * progress; // linear assumption
}

PaceTag Advisor::paceTag(const Profile& p, const Ledger& l,
                         const std::string& cat) const {
    const Category* c = p.category(cat);
    if (!c || c->budget <= 0) return {"-", 0};
    double spent = l.spent(cat);
    double daysLeft = l.daysRemaining(p);
    double daysElapsed = std::max(1.0, l.daysElapsed());
    bool earlyCycle = l.daysElapsed() < 1.0; // not enough signal for pace yet
    double rate = spent / daysElapsed;
    double projected = rate * (daysElapsed + daysLeft);

    if (spent > c->budget) return {"OVER", 2};
    if (earlyCycle) return {spent > 0 ? "started" : "-", 0};
    if (rate > 0) {
        double daysToEmpty = (c->budget - spent) / rate;
        if (daysToEmpty < daysLeft - 0.5) {
            int d = (int)std::round(daysToEmpty);
            return {std::to_string(d) + "d left", 2};
        }
    }
    double expected = c->budget * (daysElapsed / (daysElapsed + daysLeft));
    if (spent > expected * 1.2 && projected > c->budget * 1.05) return {"fast", 1};
    if (spent < expected * 0.8) return {"under", 0};
    return {"on pace", 0};
}

std::vector<Insight> Advisor::analyze(const Profile& p, const Ledger& l) const {
    std::vector<Insight> out;
    double daysElapsed = std::max(1.0, l.daysElapsed());
    bool earlyCycle = l.daysElapsed() < 1.0; // suppress pace noise on day 0
    double daysLeft = l.daysRemaining(p);
    double cycleLen = daysElapsed + daysLeft;
    double prog = daysElapsed / cycleLen;

    struct Proj { std::string cat; double spent, budget, projected, overshoot; };
    std::vector<Proj> projs;
    double totalProjected = 0;

    for (auto& c : p.categories) {
        double spent = l.spent(c.name);
        double rate = spent / daysElapsed;
        double projected = rate * cycleLen;
        double overshoot = std::max(0.0, projected - c.budget);
        projs.push_back({c.name, spent, c.budget, projected, overshoot});
        totalProjected += projected;

        // --- Forecast: will run out before cycle ends ---
        if (!earlyCycle && spent > 0 && spent <= c.budget && rate > 0) {
            double daysToEmpty = (c.budget - spent) / rate;
            if (daysToEmpty < daysLeft - 0.5) {
                double shortBy = daysLeft - daysToEmpty;
                out.push_back({Insight::Forecast, c.name,
                    c.name + ": at this pace you'll exhaust the budget in ~" +
                    text::money(daysToEmpty,0) + " days -- about " +
                    text::money(shortBy,0) + " days short. (projected " +
                    text::money(projected,0) + " / " + text::money(c.budget,0) +
                    ", " + text::money(100.0*projected/c.budget,0) + "%)",
                    2});
            }
        }
        // --- Already over ---
        if (spent > c.budget) {
            out.push_back({Insight::Overspend, c.name,
                c.name + " is over budget by " +
                text::money(spent - c.budget,0) + " " + p.currency + ".",
                2});
        }
        // --- Historical pace comparison ---
        double baseline = historicalBaselineAt(l, c.name, prog);
        if (!earlyCycle && baseline > 0) {
            double ratio = spent / baseline;
            if (ratio > 1.2) {
                out.push_back({Insight::Pace, c.name,
                    c.name + " is running " +
                    text::money((ratio-1.0)*100.0,0) +
                    "% hotter than your usual day-" +
                    text::money(daysElapsed,0) + " spend.",
                    1});
            } else if (ratio < 0.7 && spent > 0) {
                out.push_back({Insight::Pace, c.name,
                    c.name + " is " +
                    text::money((1.0-ratio)*100.0,0) +
                    "% below your usual pace -- nice.",
                    0});
            }
        }
        // --- Driver: which item is eating this category ---
        if (!earlyCycle && projected > c.budget * 1.1 && spent > 0) {
            auto bd = l.itemBreakdown(c.name);
            std::string topItem; double topVal = 0;
            for (auto& [it, v] : bd) if (v > topVal) { topVal = v; topItem = it; }
            if (!topItem.empty() && topVal / spent > 0.35) {
                out.push_back({Insight::Driver, c.name,
                    topItem + " is driving " +
                    text::money(100.0*topVal/spent,0) + "% of " + c.name + " spend.",
                    1});
            }
        }
    }

    // --- Rebalance plan: only when total is projected over and >=2 cats over ---
    double totalBudget = p.totalBudget();
    double totalOvershoot = totalProjected - totalBudget;
    int overCount = 0;
    for (auto& pr : projs) if (pr.overshoot > 0.5) ++overCount;

    if (!earlyCycle && totalOvershoot > 0.5 && overCount >= 2 && daysLeft > 0.5) {
        // Trim each over-budget category's *future* spend so projected == budget.
        // future_i = rate_i * daysLeft = projected_i * (daysLeft/cycleLen)
        // need to remove overshoot_i from future_i  ->  trimPct_i = overshoot_i / future_i
        std::string msg = "Rebalance: ";
        std::vector<std::string> parts;
        double recovered = 0;
        for (auto& pr : projs) {
            if (pr.overshoot <= 0.5) continue;
            double future = pr.projected * (daysLeft / cycleLen);
            if (future <= 0) continue;
            double trimPct = std::min(1.0, pr.overshoot / future);
            double saved = trimPct * future;
            recovered += saved;
            parts.push_back("trim " + pr.cat + " by " +
                text::money(trimPct*100.0,0) + "% (~" +
                text::money(saved,0) + " " + p.currency + ")");
        }
        msg += text::join(parts, ", ");
        msg += " to land within " + text::money(totalBudget,0) + " " + p.currency;

        // Suggest a donor if one exists with real slack.
        std::string donor; double slack = 0;
        for (auto& pr : projs) {
            double s = pr.budget - pr.projected;
            if (s > slack) { slack = s; donor = pr.cat; }
        }
        if (!donor.empty() && slack > totalOvershoot * 0.3) {
            msg += ". Or move " + text::money(std::min(slack,totalOvershoot),0) +
                   " from " + donor + " (you're under there).";
        }
        out.push_back({Insight::Rebalance, "", msg, 2});
    }

    if (out.empty()) {
        out.push_back({Insight::Praise, "",
            "Everything is on or under pace. Keep going.", 0});
    }

    // Highest severity first.
    std::stable_sort(out.begin(), out.end(),
        [](const Insight& a, const Insight& b){ return a.severity > b.severity; });
    return out;
}
