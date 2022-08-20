#pragma once
#include <cassert>
#include <cmath>  // abs
#include <sstream>
#include <string>
#include <vector>
using std::vector, std::string;
//* decides where to place the axis tics, formats the text labels */
class axisTics {
   public:
    // represents an axis tic value, facing the problem there may not be enough space to print them all
    struct ticVal {
       public:
        ticVal(double val, int64_t quantArg) : val(val), quant(quantArg) {
            if (quantArg == 0) {
                decimationLevel = std::numeric_limits<size_t>::max();
                return;
            }
            decimationLevel = 0;
            while (quantArg % 10 == 0) {
                decimationLevel += 3;
                quantArg /= 10;
            }
            if (quantArg % 5 == 0) {
                decimationLevel += 2;
                return;  // rules out 2 as 10=5*2
            }
            if (quantArg % 2 == 0)
                decimationLevel += 1;
        }
        // value of an axis tic, to be printed if there is space
        double val;
        // multiple of axis tic resolution grid
        uint64_t quant;
        // the higher the decimation level, the more fundamental.
        size_t decimationLevel;
    };

    //* Determines axis grid. Returns vector with minorTic, majorTic spacings
    static vector<double> getTicDelta(double startVal, double endVal) {
        double range = std::abs(endVal - startVal);
        double x = 1;
        while (x < range)
            x *= 10;
        vector<double> r;
        double nTarget = 4;  // minimum number
        while (r.size() < 2) {
            if (nTarget * x < range)
                r.push_back(x);
            if (nTarget * x * 0.5 < range)
                r.push_back(x * 0.5);
            else if (nTarget * x * 0.2 < range)
                r.push_back(x * 0.2);
            x /= 10;
            assert(x != 0);
        }
        return r;
    }

    //* given a spacing, calculate the absolute tic values */
    static vector<ticVal> getTicVals(double startVal, double endVal, double ticDelta) {
        vector<ticVal> r;
        if (ticDelta == 0) {
            r.push_back(ticVal(0.0, 0));
            return r;
        }

        assert(startVal < endVal);

        // quantize to grid
        int64_t quantStart = std::floor(startVal / ticDelta);
        int64_t quantEnd = std::ceil(endVal / ticDelta);
        assert((quantEnd - quantStart < 100) && "requesting excessive number of linear axis tics");

        for (int64_t quant = quantStart; quant <= quantEnd; ++quant) {
            double tic = ticDelta * quant;
            // note: don't make the interval wider, otherwise an axis looks like a tic when it's not.
            if ((tic >= startVal - 0.001 * ticDelta) && (tic <= endVal + 0.001 * ticDelta)) {
                // === add gridVal as a tic division ===
                if (!quant)
                    r.push_back(ticVal(0.0, quant));  // avoid roundoff error at origin
                else
                    r.push_back(ticVal(tic, quant));
            }
        }
        return r;
    }

    static vector<string> formatTicVals(const vector<ticVal>& ticVals, double ticDelta) {
        size_t precision = 0;
        while (precision < 18) {
            if (ticDelta > 0.9999)  // 1, 2 or 5
                break;
            ++precision;
            ticDelta *= 10;
        }

        vector<string> r;
        for (const ticVal& tv : ticVals) {
            std::stringstream ss;
            ss << std::fixed;
            ss.precision(precision);
            ss << tv.val;
            r.push_back(ss.str());
        }
        return r;
    }

   protected:
    //* test whether val lies within lim1..lim2 range, regardless of order (endpoints inclusive) */
    static inline bool isInRange(double lim1, double lim2, double val) {
        return ((val >= lim1) && (val <= lim2)) || ((val >= lim2) && (val <= lim1));
    }
};