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

vector<string> testcase9() {
    vector<float> x;
    vector<float> y;
    vector<uint16_t> mask;
    vector<string> annotNS;
    vector<string> annotWE;
    vector<uint32_t> annotIndir;
    vector<string> annotIndirText{"Point lies in the northwest quadrant", "located northeast", "point in southwest quadrant", "this point is southeast"};
    for (double phi = 0; phi < 2.0 * 3.14159265; phi += 1e-4) {
        x.push_back(std::cos(phi));
        y.push_back(std::sin(phi));
        uint16_t maskVal = x.back() < 0 ? 0 : 1;
        maskVal += y.back() < 0 ? 0 : 2;
        mask.push_back(maskVal);
        annotNS.push_back(y.back() < 0 ? "n" : "s");
        annotWE.push_back(x.back() < 0 ? "w" : "e");
        annotIndir.push_back((x.back() < 0 ? 0 : 1) + (y.back() < 0 ? 2 : 0));
    }

    std::filesystem::create_directory("testdata");
    aCCb::binaryIo::vec2file("testdata/x.float", x);
    aCCb::binaryIo::vec2file("testdata/y.float", y);
    aCCb::binaryIo::vec2file("testdata/mask.uint16", mask);
    aCCb::binaryIo::vec2file("testdata/annotNS.txt", annotNS);
    aCCb::binaryIo::vec2file("testdata/annotWE.txt", annotWE);
    aCCb::binaryIo::vec2file("testdata/annotIndir.uint32", annotIndir);
    aCCb::binaryIo::vec2file("testdata/annotIndir.txt", annotIndirText);

    vector<string> r;
    r.push_back("-title");
    r.push_back("please press 'm' to enable marker");
    for (int maskVal = 0; maskVal < 4; ++maskVal) {
        r.push_back("-trace");

        r.push_back("-dataX");
        r.push_back("testdata/x.float");

        r.push_back("-dataY");
        r.push_back("testdata/y.float");

        r.push_back("-mask");
        r.push_back("testdata/mask.uint16");

        r.push_back(std::to_string(maskVal));
        r.push_back("-marker");
        r.push_back((maskVal == 0)   ? "gx1"
                    : (maskVal == 1) ? "bx2"
                    : (maskVal == 2) ? "rx2"
                                     : "yx2");

        r.push_back("-annot");
        r.push_back("testdata/annotNS.txt");

        r.push_back("-annot");
        r.push_back("testdata/annotWE.txt");

        r.push_back("-annot2");
        r.push_back("testdata/annotIndir.uint32");
        r.push_back("testdata/annotIndir.txt");
    }
    return r;
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
        case 8:
            // lines without data (bypasses stencil, plots directly)
            r.clear();
            r.push_back("-trace");
            for (double y = 1; y <= 1024; y *= 2) {
                r.push_back("-horLineY");
                r.push_back(std::to_string(y));
            }
            r.push_back("-marker");
            r.push_back("y.2");

            r.push_back("-trace");
            for (double x = 1; x <= 1024; x *= 2) {
                r.push_back("-vertLineX");
                r.push_back(std::to_string(x * 0.001));
            }
            r.push_back("-marker");
            r.push_back("w.3");
            return r;
        case 9:
            return testcase9();
        default:
            throw runtime_error("invalid testcase number: " + std::to_string(tcNum));
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
    vector<string> args = testcase0(tcNum);

    fooplotCmdLineArgRoot l;
    std::cout << "arguments: " << std::endl;
    for (const string& a : args) {
        if (!l.acceptArg(a))
            throw aCCb::argObjException("testcase: unexpected argument '" + a + "'");
        std::cout << a << " ";
    }
    std::cout << std::endl;

    return l;
}
