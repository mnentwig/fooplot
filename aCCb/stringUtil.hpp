#include <algorithm>
#include <string>
namespace aCCb {
bool caseInsensitiveStringCompare(const std::string &str1, const std::string &str2) {
    std::string str1Cpy(str1);
    std::string str2Cpy(str2);
    std::transform(str1Cpy.begin(), str1Cpy.end(), str1Cpy.begin(), ::tolower);
    std::transform(str2Cpy.begin(), str2Cpy.end(), str2Cpy.begin(), ::tolower);
    return str1Cpy == str2Cpy;
}
}  // namespace aCCb