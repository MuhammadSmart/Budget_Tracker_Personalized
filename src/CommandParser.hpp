#pragma once
#include <string>
#include <vector>

class Profile;

struct Command {
    enum Verb {
        Log, Show, ShowCat, SetBudget, SetCurrency, SetPeriod,
        AddCategory, AddItem, Remove, History, Options, Reset,
        Help, Quit, Unknown
    };
    Verb verb = Unknown;
    std::string category;
    std::string item;
    double amount = 0.0;
    std::string note;
    std::string error;      // populated for Unknown
    std::string suggestion; // "did you mean ...?"
};

class CommandParser {
public:
    Command parse(const std::string& line, const Profile& profile) const;
};
