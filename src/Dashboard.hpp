#pragma once
#include <iosfwd>
#include <vector>
#include <string>

class Profile;
class Ledger;
class Advisor;
struct Insight;

class Dashboard {
public:
    void render(const Profile&, const Ledger&, const Advisor&,
                const std::vector<Insight>&, std::ostream&) const;
    void renderCategory(const Profile&, const Ledger&,
                        const std::string& cat, std::ostream&) const;
    void renderHistory(const Profile&, const Ledger&, std::ostream&) const;
    void renderOptions(const Profile&, std::ostream&) const;
    void renderHelp(std::ostream&) const;
};
