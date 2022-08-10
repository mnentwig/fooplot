#pragma once
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
using std::runtime_error;

#include "../aCCb/binIo.hpp"
#include "cmdLineProcessor.hpp"

const string getTempDir() {
    return "c:/temp";
}

template <typename T>
string testcase_createDatafile0() {
    if constexpr (std::is_same_v<T, uint8_t>) {
        string fname = getTempDir() + "/y.uint8";
        vector<T> vec(256);
        std::iota(vec.begin(), vec.end(), 0);
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, int8_t>) {
        string fname = getTempDir() + "/y.int8";
        vector<T> vec(256);
        std::iota(vec.begin(), vec.end(), -128);
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        string fname = getTempDir() + "/y.uint16";
        vector<T> vec(65536);
        std::iota(vec.begin(), vec.end(), 0);
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        string fname = getTempDir() + "/y.int16";
        vector<T> vec(65536);
        std::iota(vec.begin(), vec.end(), -32768);
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        string fname = getTempDir() + "/y.uint32";
        vector<T> vec;
        for (uint64_t v = std::numeric_limits<T>::min(); v < std::numeric_limits<T>::max(); v += 65536)
            vec.push_back((T)v);
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        string fname = getTempDir() + "/y.int32";
        vector<T> vec;
        for (uint64_t v = 0; v < std::numeric_limits<T>::max(); v += 65536)
            vec.push_back((T)v);
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, float>) {
        string fname = getTempDir() + "/y.float";
        vector<T> vec;
        double v = std::numeric_limits<T>::min();  // minimum positive non-denormalized value
        while (v < std::numeric_limits<T>::max()) {
            vec.push_back(v);
            v *= 2;
        }
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else if constexpr (std::is_same_v<T, double>) {
        string fname = getTempDir() + "/y.double";
        vector<T> vec;
        // note: range taken from float as the plot uses float precision
        double v = std::numeric_limits<float>::min();  // minimum positive non-denormalized value
        while (v < std::numeric_limits<float>::max()) {
            vec.push_back(v);
            v *= 2;
        }
        aCCb::binaryIo::vec2file(fname, vec);
        return fname;
    } else {
        throw runtime_error("invalid testcase type");
    }
}

vector<string> testcase0(int tcNum) {
    vector<string> r;
    string t;
    r.push_back("-trace");
    r.push_back("-dataY");
    switch (tcNum) {
        case 0:
            r.push_back(testcase_createDatafile0<uint8_t>());
            t = "uint8";
            break;
        case 1:
            r.push_back(testcase_createDatafile0<int8_t>());
            t = "int8";
            break;
        case 2:
            r.push_back(testcase_createDatafile0<uint16_t>());
            t = "uint16";
            break;
        case 3:
            r.push_back(testcase_createDatafile0<int16_t>());
            t = "int16";
            break;
        case 4:
            r.push_back(testcase_createDatafile0<uint32_t>());
            t = "uint32";
            break;
        case 5:
            r.push_back(testcase_createDatafile0<int32_t>());
            t = "int32";
            break;
        case 6:
            r.push_back(testcase_createDatafile0<float>());
            t = "float";
            break;
        case 7:
            r.push_back(testcase_createDatafile0<double>());
            t = "double";
            break;
    }
    r.push_back("-marker");
    r.push_back("g.2");
    r.push_back("-title");
    r.push_back(t + " test");
    if (tcNum > 4) {
        r.push_back("-fontsize");
        r.push_back(std::to_string(3 * tcNum));
    }
    return r;
}

fooplotCmdLineArgRoot testcase(int tcNum) {
    std::cout << "testcase " << tcNum << std::endl;
    vector<string> args;
    if (tcNum <= 7) {
        args = testcase0(tcNum);
    } else {
        throw runtime_error("invalid testcase number");
    }

    fooplotCmdLineArgRoot l;
    for (const string& a : args) {
        std::cout << "parsing " << a << std::endl;
        if (!l.acceptArg(a))
            throw aCCb::argObjException("testcase: unexpected argument '" + a + "'");
    }
    return l;
}
