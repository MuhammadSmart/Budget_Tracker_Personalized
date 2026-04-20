#include "BudgetApp.hpp"
#include "util/Text.hpp"
#include "util/Date.hpp"

BudgetApp::BudgetApp(Storage storage, std::istream& in, std::ostream& out)
    : storage_(std::move(storage)), in_(in), out_(out) {
    if (!storage_.profileExists()) {
        profile_ = Profile::fromOnboarding(in_, out_);
        storage_.saveProfile(profile_);
        ledger_.cycleStart = profile_.cycleStart;
        storage_.saveLedger(ledger_);
    } else {
        profile_ = storage_.loadProfile();
        ledger_ = storage_.loadLedger(profile_);
    }
    int rolled = ledger_.rolloverIfNeeded(profile_);
    if (rolled > 0) {
        storage_.saveProfile(profile_);
        storage_.saveLedger(ledger_);
        out_ << "(archived " << rolled << " completed cycle"
             << (rolled>1?"s":"") << " to history)\n";
    }
}

void BudgetApp::refresh() {
    auto insights = advisor_.analyze(profile_, ledger_);
    dash_.render(profile_, ledger_, advisor_, insights, out_);
}

int BudgetApp::run() {
    refresh();
    out_ << "type 'help' for commands\n\n";
    std::string line;
    while (running_) {
        out_ << "> " << std::flush;
        if (!std::getline(in_, line)) break;
        Command c = parser_.parse(line, profile_);
        handle(c);
    }
    out_ << "bye.\n";
    return 0;
}

void BudgetApp::handle(const Command& c) {
    switch (c.verb) {
    case Command::Quit:
        running_ = false; return;

    case Command::Help:
        dash_.renderHelp(out_); return;

    case Command::Options:
        dash_.renderOptions(profile_, out_); return;

    case Command::History:
        dash_.renderHistory(profile_, ledger_, out_); return;

    case Command::Show:
        refresh(); return;

    case Command::ShowCat:
        dash_.renderCategory(profile_, ledger_, c.category, out_); return;

    case Command::Log: {
        Transaction t;
        t.ts = dates::now();
        t.category = c.category;
        t.item = c.item;
        t.amount = c.amount;
        t.note = c.note;
        ledger_.log(t);
        storage_.appendTransaction(t);
        out_ << "  logged: " << (c.item.empty()?c.category:c.item)
             << " " << text::money(c.amount,2) << " " << profile_.currency
             << " -> " << c.category << "\n";
        refresh();
        return;
    }

    case Command::SetBudget: {
        Category* cat = profile_.category(c.category);
        if (!cat) { out_ << "  (no such category: " << c.category << ")\n"; return; }
        cat->budget = c.amount;
        storage_.saveProfile(profile_);
        out_ << "  " << c.category << " budget -> " << text::money(c.amount)
             << " " << profile_.currency << "\n";
        refresh(); return;
    }

    case Command::SetCurrency:
        profile_.currency = c.note;
        for (auto& ch : profile_.currency) ch = std::toupper((unsigned char)ch);
        storage_.saveProfile(profile_);
        out_ << "  currency -> " << profile_.currency << "\n"; return;

    case Command::SetPeriod: {
        Period np = (text::lower(c.note).rfind("w",0)==0)?Period::Weekly:Period::Monthly;
        if (np != profile_.period) {
            profile_.period = np;
            ledger_.forceRollover(profile_);
            storage_.saveProfile(profile_);
            storage_.saveLedger(ledger_);
            out_ << "  period -> " << (np==Period::Weekly?"weekly":"monthly")
                 << " (started a fresh cycle)\n";
            refresh();
        } else out_ << "  already " << c.note << "\n";
        return;
    }

    case Command::AddCategory: {
        if (profile_.category(c.category)) { out_<<"  (already exists)\n"; return; }
        Category nc; nc.name=text::lower(c.category); nc.budget=c.amount;
        profile_.categories.push_back(std::move(nc));
        storage_.saveProfile(profile_);
        out_ << "  added category " << c.category << " (" << text::money(c.amount)
             << " " << profile_.currency << ")\n";
        refresh(); return;
    }

    case Command::AddItem: {
        Category* cat = profile_.category(c.category);
        if (!cat) { out_<<"  (no such category)\n"; return; }
        cat->items.push_back({text::lower(c.item),{}});
        storage_.saveProfile(profile_);
        out_ << "  added item " << c.item << " -> " << c.category << "\n"; return;
    }

    case Command::Remove: {
        if (!c.item.empty()) {
            Category* cat = profile_.category(c.category);
            if (cat) {
                cat->items.erase(
                    std::remove_if(cat->items.begin(),cat->items.end(),
                        [&](const Item&i){return i.name==c.item;}),
                    cat->items.end());
                storage_.saveProfile(profile_);
                out_ << "  removed item " << c.item << " from " << c.category << "\n";
            }
        } else {
            profile_.categories.erase(
                std::remove_if(profile_.categories.begin(),profile_.categories.end(),
                    [&](const Category&x){return x.name==c.category;}),
                profile_.categories.end());
            storage_.saveProfile(profile_);
            out_ << "  removed category " << c.category << "\n";
            refresh();
        }
        return;
    }

    case Command::Reset:
        ledger_.forceRollover(profile_);
        storage_.saveProfile(profile_);
        storage_.saveLedger(ledger_);
        out_ << "  cycle closed and archived. fresh cycle started.\n";
        refresh(); return;

    case Command::Unknown:
        out_ << "  " << c.error;
        if (!c.suggestion.empty()) out_ << "  (did you mean '" << c.suggestion << "'?)";
        out_ << "\n  type 'help' for examples.\n";
        return;
    }
}
