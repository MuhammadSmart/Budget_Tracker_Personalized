#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>

namespace text {

inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, delim)) out.push_back(trim(tok));
    return out;
}

inline std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (ss >> tok) out.push_back(tok);
    return out;
}

inline std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) out += sep;
        out += v[i];
    }
    return out;
}

inline int levenshtein(const std::string& a, const std::string& b) {
    size_t n = a.size(), m = b.size();
    std::vector<int> prev(m + 1), cur(m + 1);
    for (size_t j = 0; j <= m; ++j) prev[j] = (int)j;
    for (size_t i = 1; i <= n; ++i) {
        cur[0] = (int)i;
        for (size_t j = 1; j <= m; ++j) {
            int cost = (a[i-1] == b[j-1]) ? 0 : 1;
            cur[j] = std::min({ prev[j] + 1, cur[j-1] + 1, prev[j-1] + cost });
        }
        std::swap(prev, cur);
    }
    return prev[m];
}

inline std::string padRight(const std::string& s, size_t w) {
    if (s.size() >= w) return s;
    return s + std::string(w - s.size(), ' ');
}

inline std::string padLeft(const std::string& s, size_t w) {
    if (s.size() >= w) return s;
    return std::string(w - s.size(), ' ') + s;
}

inline std::string money(double v, int prec = 0) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(prec) << v;
    return os.str();
}

// Render a progress bar. ratio may exceed 1.0 (overflow shown with a different glyph).
inline std::string bar(double ratio, int width = 20) {
    if (ratio < 0) ratio = 0;
    double capped = std::min(ratio, 1.0);
    int filled = (int)std::round(capped * width);
    int over = 0;
    if (ratio > 1.0) {
        over = std::min(width - filled + 2, (int)std::round((ratio - 1.0) * width));
        if (over < 1) over = 1;
    }
    std::string s = "[";
    for (int i = 0; i < filled; ++i) s += "#";
    for (int i = filled; i < width; ++i) s += ".";
    s += "]";
    if (over > 0) {
        for (int i = 0; i < over && i < 3; ++i) s += "!";
    }
    return s;
}

inline bool parseDouble(const std::string& s, double& out) {
    std::string clean;
    for (char c : s) if (c != ',') clean += c;
    try {
        size_t idx = 0;
        out = std::stod(clean, &idx);
        return idx == clean.size();
    } catch (...) { return false; }
}

} // namespace text
