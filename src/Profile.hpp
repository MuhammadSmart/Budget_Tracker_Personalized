#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <iosfwd>

enum class Period { Weekly, Monthly };

struct Item {
    std::string name;
    std::vector<std::string> aliases;
};

struct Category {
    std::string name;
    double budget = 0.0;
    std::vector<Item> items;
};

class Profile {
public:
    Period period = Period::Monthly;
    std::string currency = "EGP";
    std::time_t cycleStart = 0;
    std::vector<Category> categories;

    struct Match {
        const Category* cat = nullptr;
        const Item* item = nullptr;
        bool exact = false;
        explicit operator bool() const { return cat != nullptr; }
    };

    double totalBudget() const;
    Category* category(const std::string& name);
    const Category* category(const std::string& name) const;

    // Fuzzy lookup: item name/alias → category name → prefix → levenshtein.
    Match find(const std::string& token) const;

    // Best fuzzy suggestion string for an unknown token (for "did you mean?").
    std::string suggest(const std::string& token) const;

    long cycleLengthSec() const;

    // Interactive first-run wizard.
    static Profile fromOnboarding(std::istream& in, std::ostream& out);
};
