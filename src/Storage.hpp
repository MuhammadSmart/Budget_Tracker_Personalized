#pragma once
#include <string>

class Profile;
class Ledger;

class Storage {
public:
    explicit Storage(std::string dir = "./data");

    bool profileExists() const;
    Profile loadProfile() const;
    void saveProfile(const Profile&) const;

    Ledger loadLedger(const Profile&) const;
    void saveLedger(const Ledger&) const;       // rewrites ledger.log + history.tsv
    void appendTransaction(const struct Transaction&) const; // fast path

private:
    std::string dir_;
    std::string profilePath() const;
    std::string ledgerPath() const;
    std::string historyPath() const;
};
