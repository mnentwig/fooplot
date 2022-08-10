#pragma once
#include <algorithm>  // count
#include <fstream>    // ifstream
#include <stdexcept>
#include <string>
#include <vector>

#include "../aCCb/stringToNum.hpp"
namespace aCCb::binaryIo {
using std::fstream;
using std::ifstream;
using std::runtime_error;
using std::string;
using std::vector;
namespace binIo = aCCb::binaryIo;
namespace details {
inline size_t getNumBinaryElem(std::istream &is, size_t elemSize) {
    is.seekg(0, std::ios_base::end);
    std::size_t nBytes = is.tellg();
    size_t nElem = nBytes / elemSize;
    if (nElem * elemSize != nBytes)
        throw runtime_error("file contains partial element");
    return nElem;
}

/** returns number of true elements in indexOp. Copied to avoid dependency on logical indexing */
inline size_t liPopcount(const vector<bool> &indexOp) {
    return std::count(indexOp.cbegin(), indexOp.cend(), true);
}

}  // namespace details

//* stream2vec: Read binary data from stream into vector */
template <class T>
vector<T> stream2vec(std::istream &is) {
    size_t nElem = binIo::details::getNumBinaryElem(is, sizeof(T));
    is.seekg(0, std::ios_base::beg);
    vector<T> retVal(nElem);
    is.read((char *)&retVal[0], nElem * sizeof(T));
    if (!is)
        throw runtime_error("read failed");
    return retVal;
}

//* stream2vec: Read binary data from stream into vector, skipping elements where the bool vector "indexOp" is false */
template <class T>
vector<T> stream2vec(std::istream &is, vector<bool> indexOp) {
    size_t nElem = binIo::details::getNumBinaryElem(is, sizeof(T));
    if (nElem != indexOp.size())
        throw runtime_error("size mismatch is vs indexOp");

    // === reserve memory ===
    size_t popcnt = binIo::details::liPopcount(indexOp);
    vector<T> retVal(popcnt);

    // === main loop ===
    auto itIndex = indexOp.cbegin();
    const auto itIndexEnd = indexOp.cend();
    size_t streamBytePos = 0;
    size_t ixRead = 0;
    while (itIndex != itIndexEnd) {
        // === skip consecutive false ===
        while ((itIndex != itIndexEnd) && !*itIndex) {
            streamBytePos += sizeof(T);
            ++itIndex;
        }

        // === count consecutive true ===
        size_t nElemToRead = 0;
        while ((itIndex != itIndexEnd) && *itIndex) {
            ++nElemToRead;
            ++itIndex;
        }

        // === read contiguous elements ===
        if (nElemToRead) {
            size_t nBytesToRead = nElemToRead * sizeof(T);
            is.seekg(streamBytePos, std::ios_base::beg);
            if (!is)
                throw runtime_error("seekg() failed");
            is.read((char *)&retVal[ixRead], nBytesToRead);
            if (!is)
                throw runtime_error("read() failed");
            ixRead += nElemToRead;
            streamBytePos += nBytesToRead;
        }

        // === done? (optimization for trailing false) ===
        if (popcnt == ixRead)
            break;
    }
    return retVal;
}

//* vec2stream: Write vector as binary into stream */
template <class T>
void vec2stream(std::ostream &os, const vector<T> &vec) {
    os.write((char *)&vec[0], vec.size() * sizeof(T));
}

//* vec2stream: Write vector as binary into stream, skipping elements where the bool vector "indexOp" is false */
template <class T>
void vec2stream(std::ostream &os, const vector<T> &vec, const vector<bool> &indexOp) {
    // === main loop ===
    auto itIndex = indexOp.cbegin();
    const auto itIndexEnd = indexOp.cend();
    size_t ixWrite = 0;

    while (itIndex != itIndexEnd) {
        // === skip consecutive false ===
        while ((itIndex != itIndexEnd) && !*itIndex) {
            ++itIndex;
            ++ixWrite;
        }

        // === count consecutive true ===
        size_t nElemToWrite = 0;
        while ((itIndex != itIndexEnd) && *itIndex) {
            ++nElemToWrite;
            ++itIndex;
        }

        // === write contiguous elements ===
        if (nElemToWrite) {
            size_t nBytesToWrite = nElemToWrite * sizeof(T);
            os.write((char *)&vec[ixWrite], nBytesToWrite);
            if (!os)
                throw runtime_error("write() failed");
            ixWrite += nElemToWrite;
        }
    }
}

// === wrappers with filename argument ===

//* file2vec: Read binary data from file into vector */
template <class T>
vector<T> file2vec(const string fname) {
    std::ifstream is(fname, std::ifstream::binary);
    if (!is)
        throw runtime_error("failed to open file");
    return binIo::stream2vec<T>(is);
}

//* file2vec: Read binary data from stream into vector, skipping elements where the bool vector "indexOp" is false */
template <class T>
vector<T> file2vec(const string fname, const vector<bool> &indexOp) {
    std::ifstream is(fname, std::ifstream::binary);
    if (!is)
        throw runtime_error("failed to open file");
    return binIo::stream2vec<T>(is, indexOp);
}

//* vec2file: Write vector as binary into file */
template <class T>
void vec2file(const string &fname, const vector<T> &vec) {
    fstream os(fname.c_str(), std::ios::out | std::ifstream::binary);
    if (!os)
        throw runtime_error("failed to open file");
    binIo::vec2stream<T>(os, vec);
}

//* vec2file: Write vector as binary into stream, skipping elements where the bool vector "indexOp" is false */
template <class T>
void vec2file(const string &fname, const vector<T> &vec, const vector<bool> &indexOp) {
    fstream os(fname.c_str(), std::ios::out | std::ifstream::binary);
    if (!os)
        throw runtime_error("failed to open file");
    binIo::vec2stream<T>(os, vec, indexOp);
}

// === equivalent for .txt ===
template <class T>
vector<T> file2vec_asc(const string fname) {
    vector<T> r;
    std::ifstream is(fname, std::ifstream::binary);
    if (!is)
        throw runtime_error("failed to open file");
    string line;
    size_t lineBase1 = 1;
    while (std::getline(is, line)) {
        T val;
        if (!aCCb::str2num(line, val)) {
            const size_t maxLen = 32;
            if (line.size() > maxLen)
                line = line.substr(0, maxLen) + "...";
            throw runtime_error(string("File '") + fname + "' line " + std::to_string(lineBase1) + ": failed to parse number from '" + line + "'");
        }
        r.push_back(val);
        ++lineBase1;
    }
    return r;
}
}  // namespace aCCb::binaryIo
