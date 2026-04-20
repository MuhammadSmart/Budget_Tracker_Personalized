#include "Dashboard.hpp"
#include "Profile.hpp"
#include "Ledger.hpp"
#include "Advisor.hpp"
#include "util/Text.hpp"
#include "util/Date.hpp"
#include <iostream>
#include <algorithm>

static const int BARW = 22;

void Dashboard::render(const Profile& p, const Ledger& l, const Advisor& adv,
                       const std::vector<Insight>& insights,
                       std::ostream& os) const {
    double cycleLen = double(p.cycleLengthSec()) / dates::DAY;
    int day = std::max(1, (int)std::round(l.daysElapsed() + 0.5));
    int len = (int)cycleLen;
    int pct = (int)std::round(l.cycleProgress(p) * 100.0);

    size_t nameW = 8;
    for (auto& c : p.categories) nameW = std::max(nameW, c.name.size());

    std::string hdr = "Budget -- cycle day " + std::to_string(day) + "/" +
                      std::to_string(len) + " (" + std::to_string(pct) + "%) -- " +
                      p.currency;
    os << "\n+" << std::string(70, '-') << "+\n";
    os << "| " << text::padRight(hdr, 68) << " |\n";
    os << "| " << std::string(68, ' ') << " |\n";

    for (auto& c : p.categories) {
        double sp = l.spent(c.name);
        double ratio = c.budget > 0 ? sp / c.budget : 0;
        PaceTag tag = adv.paceTag(p, l, c.name);
        std::string mark = tag.severity>=2 ? "!!" : tag.severity==1 ? "! " : "  ";
        std::string row =
            text::padRight(c.name, nameW) + " " +
            text::bar(ratio, BARW) + " " +
            text::padLeft(text::money(sp), 5) + " /" +
            text::padLeft(text::money(c.budget), 5) + "  " +
            text::padLeft(text::money(ratio*100.0,0)+"%", 5) + "  " +
            mark + tag.glyph;
        os << "| " << text::padRight(row, 68) << " |\n";

        // top items inline (max 4)
        auto bd = l.itemBreakdown(c.name);
        if (!bd.empty()) {
            std::vector<std::pair<std::string,double>> v(bd.begin(), bd.end());
            std::sort(v.begin(), v.end(),
                [](auto&a,auto&b){return a.second>b.second;});
            std::string sub = "  ";
            for (size_t i=0;i<v.size()&&i<4;++i) {
                if (i) sub += "  .  ";
                sub += v[i].first + " " + text::money(v[i].second);
            }
            os << "| " << text::padRight("  " + sub, 68) << " |\n";
        }
    }

    os << "| " << text::padRight(std::string(66,'-'), 68) << " |\n";
    double tsp = l.spentTotal(), tb = p.totalBudget();
    double tratio = tb>0 ? tsp/tb : 0;
    std::string trow =
        text::padRight("TOTAL", nameW) + " " +
        text::bar(tratio, BARW) + " " +
        text::padLeft(text::money(tsp), 5) + " /" +
        text::padLeft(text::money(tb), 5) + "  " +
        text::padLeft(text::money(tratio*100.0,0)+"%", 5);
    os << "| " << text::padRight(trow, 68) << " |\n";
    os << "| " << std::string(68, ' ') << " |\n";

    // top 3 insights
    int shown=0;
    for (auto& in : insights) {
        if (shown>=3) break;
        std::string pre = in.severity>=2 ? "!! " : in.severity==1 ? " ! " : " > ";
        std::string msg = pre + in.message;
        // wrap to 66 cols
        while (!msg.empty()) {
            std::string chunk = msg.substr(0, 66);
            if (msg.size()>66) {
                size_t cut = chunk.find_last_of(' ');
                if (cut!=std::string::npos && cut>40) chunk = chunk.substr(0,cut);
            }
            os << "| " << text::padRight(chunk, 68) << " |\n";
            msg = (chunk.size()<msg.size()) ? "   "+text::trim(msg.substr(chunk.size())) : "";
        }
        ++shown;
    }
    os << "+" << std::string(70, '-') << "+\n\n";
}

void Dashboard::renderCategory(const Profile& p, const Ledger& l,
                               const std::string& cat, std::ostream& os) const {
    const Category* c = p.category(cat);
    if (!c) { os << "(no such category: " << cat << ")\n"; return; }
    double sp = l.spent(cat);
    os << "\n" << c->name << " -- " << text::money(sp) << " / "
       << text::money(c->budget) << " " << p.currency << "  ("
       << text::money(c->budget>0?100.0*sp/c->budget:0,0) << "%)\n";
    auto bd = l.itemBreakdown(cat);
    std::vector<std::pair<std::string,double>> v(bd.begin(),bd.end());
    std::sort(v.begin(),v.end(),[](auto&a,auto&b){return a.second>b.second;});
    for (auto& [it,val] : v)
        os << "  " << text::padRight(it,14) << text::padLeft(text::money(val),8)
           << "  " << text::bar(c->budget>0?val/c->budget:0, 14) << "\n";
    if (v.empty()) os << "  (nothing logged yet)\n";
    os << "\n";
}

void Dashboard::renderHistory(const Profile& p, const Ledger& l,
                              std::ostream& os) const {
    if (l.history.empty()) { os << "\n(no closed cycles yet)\n\n"; return; }
    os << "\nPast cycles:\n";
    for (auto it = l.history.rbegin(); it != l.history.rend(); ++it) {
        double tsp=0, tb=0;
        for (auto&[k,v]:it->spent) tsp+=v;
        for (auto&[k,v]:it->budget) tb+=v;
        os << "  " << dates::fmt(it->start) << " -> " << dates::fmt(it->end)
           << "   spent " << text::padLeft(text::money(tsp),6)
           << " / " << text::money(tb) << " " << p.currency
           << "  (" << text::money(tb>0?100.0*tsp/tb:0,0) << "%)\n";
    }
    os << "\n";
}

void Dashboard::renderOptions(const Profile& p, std::ostream& os) const {
    os << "\nOptions:\n"
       << "  currency     : " << p.currency << "       (set currency <X>)\n"
       << "  period       : " << (p.period==Period::Weekly?"weekly":"monthly")
       << "    (set period weekly|monthly)\n"
       << "  cycle start  : " << dates::fmt(p.cycleStart) << "\n"
       << "  categories   : " << p.categories.size() << "\n"
       << "  total budget : " << text::money(p.totalBudget()) << " " << p.currency << "\n\n";
}

void Dashboard::renderHelp(std::ostream& os) const {
    os << "\nThings you can type (free-form, any order):\n"
       << "  bought eggs 45            log a purchase (item -> category)\n"
       << "  spent 32 on uber          same, different phrasing\n"
       << "  rice 90                   verb optional\n"
       << "  show / status             redraw dashboard\n"
       << "  show groceries            item breakdown for one category\n"
       << "  set groceries 900         change a category budget\n"
       << "  set currency USD          change display currency\n"
       << "  set period weekly         switch cycle length\n"
       << "  add category health 400   new category\n"
       << "  add item coffee to misc   new item under a category\n"
       << "  history                   past cycles\n"
       << "  options                   settings\n"
       << "  reset                     close current cycle now\n"
       << "  help / quit\n\n";
}
