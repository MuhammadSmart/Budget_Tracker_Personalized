#pragma once
#include <ctime>
#include <string>

namespace dates {

constexpr long DAY = 24 * 60 * 60;

inline std::time_t now() { return std::time(nullptr); }

inline long cycleLengthSec(bool weekly) {
    return weekly ? 7 * DAY : 30 * DAY; // monthly = rolling 30 days
}

inline double daysBetween(std::time_t a, std::time_t b) {
    return double(b - a) / double(DAY);
}

inline std::string fmt(std::time_t t) {
    char buf[32];
    std::tm tm = *std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

} // namespace dates
