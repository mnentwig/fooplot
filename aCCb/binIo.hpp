#pragma once
#include <fstream>  // ifstream
#include <stdexcept>
#include <string>
#include <vector>

namespace aCCb::binaryIo {
using std::fstream;
using std::ifstream;
using std::runtime_error;
using std::string;
using std::vector;
namespace binIo = aCCb::binaryIo;
//* vec2stream: Write vector as binary into stream */
template <class T>
void vec2stream(std::ostream &os, const vector<T> &vec) {
    os.write((char *)&vec[0], vec.size() * sizeof(T));
}
//* vec2file: Write vector as binary into file */
template <class T>
void vec2file(const string &fname, const vector<T> &vec) {
    fstream os(fname.c_str(), std::ios::out | std::ifstream::binary);
    if (!os)
        throw runtime_error("failed to open file");
    binIo::vec2stream<T>(os, vec);
}

//* vec2file: Write string vector into stream */
template <>
void vec2file(const string &fname, const vector<string> &vec) {
    fstream os(fname.c_str(), std::ios::out | std::ifstream::binary);
    if (!os)
        throw runtime_error("failed to open file");
    for (auto s : vec)
        os << s << "\n";
}

}  // namespace aCCb::binaryIo
