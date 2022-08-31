#pragma once
#include <cassert>
#include <stdexcept>
#include <string>
namespace aCCb {
using std::string;
inline int stoi(string val) {
    size_t ix;
    int retval;
    try {
        retval = std::stoi(val, &ix);
    } catch (std::exception& e) {
        throw e;  // throw from "foreign" binary code is not reliably caught in gdb => re-throw
    }
    while (ix < val.length()) {
        switch (val[ix]) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
                ++ix;
                continue;
            default:
                throw std::runtime_error("conversion string to number failed");
        }
    }
    return retval;
}

//** safe conversion of string to number */
template <typename T>
static inline bool str2num(const string& str, T& val) {
    char* endptr = NULL;
    errno = 0;
    T valB;  // temporary result: Update output parameter only on success
    if constexpr (std::is_unsigned_v<T> && std::is_integral_v<T>) {
        unsigned long long v = strtoull(str.c_str(), &endptr, /*base*/ 10);
        valB = v;
        if ((unsigned long long)valB != v)
            return false;  // out of range
    } else if constexpr (std::is_signed_v<T> && std::is_integral_v<T>) {
        long long v = strtoll(str.c_str(), &endptr, /*base*/ 10);
        valB = v;
        if ((long long)valB != v)
            return false;  // out of range
    } else if constexpr (std::is_floating_point_v<T> && (sizeof(T) == 4)) {
        valB = strtof(str.c_str(), &endptr);
    } else if constexpr (std::is_floating_point_v<T> && (sizeof(T) == 8)) {
        valB = strtod(str.c_str(), &endptr);
    } else
        return false;  // improper use
    if (errno != 0)
        return false;  // conversion error;
    while (*endptr)
        if (!std::isspace(*endptr++))
            return false;
    val = valB;
    return true;
}

}  // namespace aCCb
