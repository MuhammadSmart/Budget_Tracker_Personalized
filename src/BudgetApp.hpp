#pragma once
#include "Profile.hpp"
#include "Ledger.hpp"
#include "Storage.hpp"
#include "Advisor.hpp"
#include "Dashboard.hpp"
#include "CommandParser.hpp"
#include <iostream>

class BudgetApp {
public:
    explicit BudgetApp(Storage storage,
                       std::istream& in = std::cin,
                       std::ostream& out = std::cout);
    int run();

private:
    void handle(const Command& c);
    void refresh();

    Storage storage_;
    std::istream& in_;
    std::ostream& out_;
    Profile profile_;
    Ledger ledger_;
    Advisor advisor_;
    Dashboard dash_;
    CommandParser parser_;
    bool running_ = true;
};
