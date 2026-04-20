#include "CommandParser.hpp"
#include "Profile.hpp"
#include "util/Text.hpp"
#include <set>

static const std::set<std::string> kFiller = {
    "i","just","the","a","an","on","for","of","my","some","at","to","in","and"
};
static const std::set<std::string> kLogVerbs = {
    "bought","buy","spent","spend","paid","pay","got","log","add","purchased"
};
static const std::set<std::string> kShowVerbs = {
    "show","status","dash","dashboard","how","ls","list"
};

Command CommandParser::parse(const std::string& raw, const Profile& prof) const {
    Command c;
    std::string line = text::trim(raw);
    if (line.empty()) { c.verb = Command::Show; return c; }

    auto toks = text::tokenize(text::lower(line));

    // ---- single-word / fixed commands ----
    std::string first = toks[0];
    if (first=="quit"||first=="q"||first=="exit"||first=="bye")
        { c.verb=Command::Quit; return c; }
    if (first=="help"||first=="?"||first=="commands")
        { c.verb=Command::Help; return c; }
    if (first=="history"||(toks.size()>=2&&first=="last"))
        { c.verb=Command::History; return c; }
    if (first=="options"||first=="settings"||first=="config")
        { c.verb=Command::Options; return c; }
    if (first=="reset" || (toks.size()>=2&&first=="new"&&toks[1]=="cycle"))
        { c.verb=Command::Reset; return c; }

    // ---- "set ..." family ----
    if (first=="set" || first=="budget" || first=="change") {
        if (toks.size()>=3 && toks[1]=="currency")
            { c.verb=Command::SetCurrency; c.note=toks[2]; return c; }
        if (toks.size()>=3 && toks[1]=="period")
            { c.verb=Command::SetPeriod; c.note=toks[2]; return c; }
        // set <category> <amount>
        double amt;
        if (toks.size()>=3 && text::parseDouble(toks.back(), amt)) {
            std::vector<std::string> mid(toks.begin()+1, toks.end()-1);
            auto m = prof.find(text::join(mid,"_"));
            if (m && !m.item) { c.verb=Command::SetBudget; c.category=m.cat->name; c.amount=amt; return c; }
            if (!mid.empty()) { c.verb=Command::SetBudget; c.category=text::join(mid,"_"); c.amount=amt; return c; }
        }
        c.verb=Command::Unknown; c.error="set what? try: set <category> <amount> | set currency X | set period weekly";
        return c;
    }

    // ---- "add category X N" / "add item X to CAT" ----
    if ((first=="add"||first=="new") && toks.size()>=2 &&
        (toks[1]=="category"||toks[1]=="cat")) {
        double amt=0;
        if (toks.size()>=4 && text::parseDouble(toks.back(), amt)) {
            std::vector<std::string> mid(toks.begin()+2, toks.end()-1);
            c.verb=Command::AddCategory; c.category=text::join(mid,"_"); c.amount=amt; return c;
        }
        c.verb=Command::Unknown; c.error="usage: add category <name> <budget>"; return c;
    }
    if ((first=="add"||first=="new") && toks.size()>=2 && toks[1]=="item") {
        // add item <name> to <category>
        size_t toIdx=0;
        for (size_t i=2;i<toks.size();++i) if (toks[i]=="to"||toks[i]=="in"){toIdx=i;break;}
        if (toIdx>2 && toIdx+1<toks.size()) {
            c.verb=Command::AddItem;
            c.item=text::join(std::vector<std::string>(toks.begin()+2,toks.begin()+toIdx),"_");
            auto m = prof.find(text::join(std::vector<std::string>(toks.begin()+toIdx+1,toks.end()),"_"));
            if (m) c.category=m.cat->name;
            else { c.verb=Command::Unknown; c.error="unknown category"; }
            return c;
        }
        c.verb=Command::Unknown; c.error="usage: add item <name> to <category>"; return c;
    }
    if (first=="remove"||first=="delete"||first=="rm") {
        if (toks.size()>=2) {
            auto m = prof.find(toks[1]);
            if (m) {
                c.verb=Command::Remove; c.category=m.cat->name;
                if (m.item) c.item=m.item->name;
                return c;
            }
        }
        c.verb=Command::Unknown; c.error="remove what?"; return c;
    }

    // ---- free-form: hunt for (verb?, number, item/category) in any order ----
    bool sawLogVerb=false, sawShowVerb=false;
    double amount=0; bool haveAmount=false;
    Profile::Match match;
    std::vector<std::string> leftover;

    for (auto& t : toks) {
        if (kFiller.count(t)) continue;
        if (kLogVerbs.count(t)) { sawLogVerb=true; continue; }
        if (kShowVerbs.count(t)) { sawShowVerb=true; continue; }
        double d;
        if (!haveAmount && text::parseDouble(t,d)) { amount=d; haveAmount=true; continue; }
        if (!match) {
            auto m = prof.find(t);
            if (m) { match=m; continue; }
        }
        leftover.push_back(t);
    }

    if (sawShowVerb && !haveAmount) {
        if (match) { c.verb=Command::ShowCat; c.category=match.cat->name; return c; }
        c.verb=Command::Show; return c;
    }

    if (haveAmount && match) {
        c.verb=Command::Log;
        c.category=match.cat->name;
        c.item = match.item ? match.item->name : "";
        c.amount=amount;
        c.note = text::join(leftover," ");
        return c;
    }
    if (haveAmount && !match) {
        c.verb=Command::Unknown;
        c.error="I see an amount ("+text::money(amount,2)+") but couldn't match an item or category.";
        if (!leftover.empty()) c.suggestion = prof.suggest(leftover[0]);
        return c;
    }
    if (match && !haveAmount && !sawLogVerb) {
        c.verb=Command::ShowCat; c.category=match.cat->name; return c;
    }

    c.verb=Command::Unknown;
    c.error="Didn't understand that.";
    if (!leftover.empty()) c.suggestion = prof.suggest(leftover[0]);
    return c;
}
