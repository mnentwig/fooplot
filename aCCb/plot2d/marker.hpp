#pragma once
#include <cassert>
#include <cmath>  // sqrt
#include <string>
#include <vector>
using std::string, std::vector;
class marker_cl {
   public:
    marker_cl(const string sequence, uint32_t rgba) : rgba(rgba) {
        int charCount = sequence.size();
        int markerSize1d = std::sqrt(charCount);

        assert(markerSize1d * markerSize1d == charCount && "marker must be square");
        int delta = markerSize1d / 2;
        assert(2 * delta + 1 == markerSize1d && "marker size must be odd");
        dxMinus = delta;
        dxPlus = delta;
        dyMinus = delta;
        dyPlus = delta;
        for (int ix = 0; ix < charCount; ++ix)
            seq.push_back(sequence[ix] != ' ');
    }

    // marker is given as const - use public fields
    int dxMinus;
    int dxPlus;
    int dyMinus;
    int dyPlus;
    vector<int> seq;
    uint32_t rgba;
};