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
    static vector<double> getTicVals(double startVal, double endVal, double ticDelta) {
        vector<double> r;
        if (ticDelta == 0) {
            r.push_back(0);
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
                    r.push_back(0);  // avoid roundoff error at origin
                else
                    r.push_back(tic);
            }
        }
        return r;
    }

    static vector<string> formatTicVals(const vector<double>& ticVals, double ticDelta) {
        size_t precision = 0;
        while (precision < 18) {
            if (ticDelta > 0.9999)  // 1, 2 or 5
                break;
            ++precision;
            ticDelta *= 10;
        }

        vector<string> r;
        for (double v : ticVals) {
            std::stringstream ss;
            ss << std::fixed;
            ss.precision(precision);
            ss << v;
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
