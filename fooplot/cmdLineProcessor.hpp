#pragma once
#include <deque>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "aCCb/cmdLineParsing.hpp"
#include "aCCb/stringToNum.hpp"
using std::vector, std::string, std::runtime_error;

// ==============================================================================
// command line parser: '-trace' structure
// ==============================================================================
class trace : public aCCb::argObj {
   public:
    trace() : argObj("-trace"), marker("gx1") {}
    bool acceptArg_stateUnset(const string &a) {
        if (std::find(switchArgs.cbegin(), switchArgs.cend(), a) != switchArgs.cend()) {
            // implement switches here
        } else if (std::find(stateArgs.cbegin(), stateArgs.cend(), a) != stateArgs.cend()) {
            state = a;
        } else {
            return argObj::acceptArg_stateUnset(a);
        };
        return true;
    }

    bool acceptArg_stateSet(const string &a) {
        if (state == "-dataX")
            dataX = std::filesystem::canonical(a).string();
        else if (state == "-dataY")
            dataY = std::filesystem::canonical(a).string();
        else if (state == "-marker")
            marker = a;
        else if (state == "-horLineY") {
            float v;
            if (!aCCb::str2num(a, v)) throw aoException(state + ": failed to parse number ('" + a + "')");
            horLineY.push_back(v);
        } else if (state == "-vertLineX") {
            float v;
            if (!aCCb::str2num(a, v)) throw aoException(state + ": failed to parse number ('" + a + "')");
            vertLineX.push_back(v);
        } else if (state == "-annot")
            annotate = a;
        else if (state == "-mask") {
            maskFile = a;
            state = "-mask (value)";
            return true;
        } else if (state == "-mask (value)") {
            if (!aCCb::str2num(a, maskVal)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else
            throw runtime_error("state implementation missing: " + state);
        state = "";
        return true;
    }

    string dataX;
    string dataY;
    string marker;
    vector<float> horLineY;
    vector<float> vertLineX;
    string maskFile;
    uint16_t maskVal;
    string annotate;

   protected:
    const vector<string> stateArgs{"-dataX", "-dataY", "-marker", "-horLineY", "-vertLineX", "-annot", "-mask"};
    const vector<string> switchArgs;
};

// ==============================================================================
// command line parser: root level
// ==============================================================================
class fooplotCmdLineArgRoot : public aCCb::argObj {
   public:
    fooplotCmdLineArgRoot() : argObj("cmdline root") {}
    bool acceptArg_stateUnset(const string &a) {
        // first pass to children (they are younger)
        if (argObj::acceptArg_stateUnset(a))
            return true;

        // handle it ourselves
        if (std::find(switchArgs.cbegin(), switchArgs.cend(), a) != switchArgs.cend()) {
            // tokens without follow-up argument
            if (a == "-trace") {
                traces.push_back(trace());  // note: container may not invalidate iterators on insertion e.g. DO NOT use vector
                stack.push_back(&traces.back());
            } else if (a == "-help") {
                showUsage = true;
            } else
                throw new runtime_error(token + " ?? missing implementation for '" + a + "'");
        } else if (std::find(stateArgs.cbegin(), stateArgs.cend(), a) != stateArgs.cend()) {
            // tokens with follow-up argument. The next input is handled by acceptArg_stateSet().
            state = a;
        } else
            return false;
        return true;  // common for most of the above
    }

    bool acceptArg_stateSet(const string &a) {
        if (state == "-title") {
            title = a;
        } else if (state == "-xlabel") {
            xlabel = a;
        } else if (state == "-ylabel") {
            ylabel = a;
        } else if (state == "-xLimLow") {
            if (!aCCb::str2num(a, xLimLow)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-xLimHigh") {
            if (!aCCb::str2num(a, xLimHigh)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-yLimLow") {
            if (!aCCb::str2num(a, yLimLow)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-yLimHigh") {
            if (!aCCb::str2num(a, yLimHigh)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-windowX") {
            if (!aCCb::str2num(a, windowX)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-windowY") {
            if (!aCCb::str2num(a, windowY)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-windowW") {
            if (!aCCb::str2num(a, windowW)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-windowH") {
            if (!aCCb::str2num(a, windowH)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-fontsize") {
            if (!aCCb::str2num(a, fontsize)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else if (state == "-sync") {
            syncfile = a;
        } else if (state == "-persist") {
            persistfile = a;
        } else if (state == "-testcase") {
            if (!aCCb::str2num(a, testcase)) throw aoException(state + ": failed to parse number ('" + a + "')");
        } else
            return argObj::acceptArg_stateSet(a);  // throws error
        state = "";
        return true;
    }

    string title;
    string xlabel;
    string ylabel;
    double xLimLow = std::numeric_limits<double>::quiet_NaN();
    double xLimHigh = std::numeric_limits<double>::quiet_NaN();
    double yLimLow = std::numeric_limits<double>::quiet_NaN();
    double yLimHigh = std::numeric_limits<double>::quiet_NaN();
    int windowX = -1;
    int windowY = -1;
    int windowW = -1;
    int windowH = -1;
    int fontsize = 13;
    string syncfile;
    string persistfile;
    std::deque<trace> traces;
    bool showUsage = false;
    int testcase = -1;

   protected:
    vector<string> stateArgs{"-title", "-xlabel", "-ylabel", "-xLimLow", "-xLimHigh", "-yLimLow", "-yLimHigh", "-sync", "-persist", "-windowX", "-windowY", "-windowW", "-windowH", "-fontsize", "-testcase"};
    vector<string> switchArgs{"-trace", "-help"};
};