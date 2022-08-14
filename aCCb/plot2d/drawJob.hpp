#pragma once
#include <FL/Fl_Image.H>
#include <FL/fl_draw.H>

#include <cassert>
#include <cmath>  // ceil
#include <future>
#include <string>
#include <vector>

#include "marker.hpp"
#include "proj.hpp"
using std::vector, std::string;
// type held by the stencil. A single bit would be sufficient but the required read/bit masking/write completely kills performance.
// benchmarking shows "byte" is fastest. This seems plausible, given that a typical memory hardware architecture supports byte-level masked write via dedicated "enable" lines
typedef uint8_t stencil_t;  // bool: 32 ms; uint8: 4.5 ms; uint16: 6 ms uint32_t: 9 ms uint64_t: 16 ms

// one "trace" (set of things to be rendered using a common marker by convolution)
class drawJob {
   protected:
    // multithreaded job description, for segmenting a trace with a large nr. of points into multiple "jobs" that are rendered to stencil in parallel
    // note: passed by value - don't put anything large inside
    class job_t {
       public:
        job_t(size_t ixStart, size_t ixEnd, const vector<float>* pDataX, const vector<float>* pDataY, const proj<float> p, const vector<uint16_t>* pMask, uint16_t maskVal, vector<stencil_t>* pStencil)
            : ixStart(ixStart),
              ixEnd(ixEnd),
              pDataX(pDataX),
              pDataY(pDataY),
              p(p),
              pMask(pMask),
              maskVal(maskVal),
              pStencil(pStencil) {
            if ((pDataX != NULL) && (pDataY != NULL))
                if (pDataX->size() != pDataY->size())
                    throw std::runtime_error("inconsistent trace data size X/Y");
        }

        const size_t ixStart;
        const size_t ixEnd;
        const vector<float>* pDataX;
        const vector<float>* pDataY;
        const proj<float> p;
        const vector<uint16_t>* pMask;
        uint16_t maskVal;
        vector<stencil_t>* pStencil;
    };

    // worker function to draw part of a trace into a stencil (parallelized)
    // template variants are separate at compile time for performance
    template <bool hasX, bool hasMask>
    static void drawDots(const job_t job) {
        const int width = job.p.getScreenWidth();
        const int height = job.p.getScreenHeight();

        float plotX;  // for !hasX, use +1.0f increments instead of repeated int-to-float conversion
        if constexpr (!hasX)
            plotX = job.ixStart + 1.0f;

        for (size_t ix = job.ixStart; ix < job.ixEnd; ++ix) {
            if (!hasMask || (*(job.pMask))[ix] == job.maskVal) {
                if constexpr (hasX)
                    plotX = (*(job.pDataX))[ix];
                int pixX = job.p.projX(plotX);
                if ((pixX >= 0) && (pixX < width)) {
                    float plotY = (*(job.pDataY))[ix];
                    int pixY = job.p.projY(plotY);
                    if ((pixY >= 0) && (pixY < height))
                        (*job.pStencil)[pixY * width + pixX] = 1;
                }  // if x in range
            }      // if mask enables point

            if constexpr (!hasX)
                plotX += 1.0f;
        }  // for ix
    }

   public:
    drawJob(const vector<float>* pDataX,
            const vector<float>* pDataY,
            const vector<const vector<string>*> pAnnot,
            const marker_cl* marker,
            vector<float> vertLineX,
            vector<float> horLineY,
            const vector<uint16_t>* pMask,
            uint16_t maskVal)
        : marker(marker),
          pDataX(pDataX),
          pDataY(pDataY),
          pAnnot(pAnnot),
          vertLineX(vertLineX),
          horLineY(horLineY),
          pMask(pMask),
          maskVal(maskVal) {}

    // draws only horizontal and vertical lines to screen without use of a stencil
    void drawLinesDirectly(const proj<double> pScreen) {
        fl_color(marker->rgba & 0xFF, (marker->rgba >> 8) & 0xFF, (marker->rgba >> 16) & 0xFF);

        // === vertical lines ===
        for (float x : vertLineX) {
            int pixX = pScreen.projX(x);
            for (int dx = -marker->dxMinus; dx <= marker->dxPlus; ++dx) {
                fl_begin_line();
                fl_vertex(pixX + dx, pScreen.getScreenY0());
                fl_vertex(pixX + dx, pScreen.getScreenY1());
                fl_end_line();
            }
        }

        // === horizontal lines ===
        for (float y : horLineY) {
            int pixY = pScreen.projY(y);
            for (int dy = -marker->dyMinus; dy <= marker->dyPlus; ++dy) {
                fl_begin_line();
                fl_vertex(pScreen.getScreenX0(), pixY + dy);
                fl_vertex(pScreen.getScreenX1(), pixY + dy);
                fl_end_line();
            }
        }
    }

    bool hasPoints() {
        return pDataY != NULL;
    }

    void drawToStencil(const proj<float> p, vector<stencil_t>& stencil) {
        // === vertical lines ===
        const int width = p.getScreenWidth();
        const int height = p.getScreenHeight();
        for (float x : vertLineX) {
            int pixX = p.projX(x);
            if ((pixX >= 0) && (pixX < width))
                for (int pixY = 0; pixY < height; ++pixY)
                    stencil[pixY * width + pixX] = 1;
        }

        // === horizontal lines ===
        for (float y : horLineY) {
            int pixY = p.projY(y);
            if ((pixY >= 0) && (pixY < height))
                for (int pixX = 0; pixX < width; ++pixX)
                    stencil[pixY * width + pixX] = 1;
        }

        // === traces ===
        if (!pDataY)
            return;
        size_t nData = pDataY->size();
        const size_t chunk = 65536 * 16;
        vector<std::future<void>> futs;
        for (size_t chunkIxStart = 0; chunkIxStart < nData; chunkIxStart += chunk) {
            size_t chunkIxEnd = std::min(chunkIxStart + chunk, nData);
            job_t job(chunkIxStart, chunkIxEnd, pDataX, pDataY, p, pMask, maskVal, &stencil);

            // === sanity check ===
            if (pDataX && pDataY && (pDataX->size() != pDataY->size()))
                throw std::runtime_error("dataX / dataY vectors differ in length");

            if (pMask && pDataY && (pMask->size() != pDataY->size()))
                throw std::runtime_error("dataY and mask differ in length");

            // each of the following variants refers to a custom variant of the performance-critical "drawDots" function that has the conditions optimized out as constexpr
            bool hasDataX = pDataX != NULL;
            bool hasMask = pMask != NULL;

            if (!hasDataX && !hasMask)
                futs.push_back(std::async(drawDots</*hasDataX*/ false, /*hasMask*/ false>, /*pass by value*/ job));
            else if (!hasDataX && hasMask)
                futs.push_back(std::async(drawDots</*hasDataX*/ false, /*hasMask*/ true>, /*pass by value*/ job));
            else if (hasDataX && !hasMask)
                futs.push_back(std::async(drawDots</*hasDataX*/ true, /*hasMask*/ false>, /*pass by value*/ job));
            else /*if (hasDataX && hasMask)*/
                futs.push_back(std::async(drawDots</*hasDataX*/ true, /*hasMask*/ true>, /*pass by value*/ job));
        }
        for (std::future<void>& f : futs)
            f.get();
    }

    static vector<stencil_t>
    convolveStencil(const vector<stencil_t>& stencil, int width, int height, const marker_cl* marker) {
        vector<stencil_t> r(stencil);  // center pixel
                                       //        return r;
        int markerSeqPos = 0;
        for (int dx = -marker->dxMinus; dx <= marker->dxPlus; ++dx) {
            for (int dy = -marker->dyMinus; dy <= marker->dyPlus; ++dy, ++markerSeqPos) {
                if ((dx == 0) && (dy == 0))
                    continue;  // center pixel is set by return value constructor
                if (marker->seq[markerSeqPos]) {
                    int ixSrc = 0;
                    int ixDest = 0;
                    int absDx = std::abs(dx);
                    int absDy = std::abs(dy);
                    if (dx < 0)
                        ixSrc += absDx;
                    if (dx > 0)
                        ixDest += absDx;
                    if (dy < 0)
                        ixSrc += absDy * width;
                    if (dy > 0)
                        ixDest += absDy * width;
                    for (int ixRow = height - absDy; ixRow != 0; --ixRow) {
                        for (int ixCol = width - absDx; ixCol != 0; --ixCol) {
                            assert(ixSrc >= 0);
                            assert(ixDest >= 0);
                            assert(ixSrc < (int)r.size());
                            assert(ixDest < (int)r.size());
                            r[ixDest] = r[ixDest] | stencil[ixSrc];
                            ++ixDest;
                            ++ixSrc;
                        }  // for column
                        ixDest += absDx;
                        ixSrc += absDx;
                    }  // for row
                }      // if marker pixel enabled
            }          // for marker column
        }              // for marker row
        return r;
    }

    static void drawStencil2rgba(const vector<stencil_t>& stencil, int width, int height, const marker_cl* marker, vector<uint32_t>& rgba) {
        assert((int)stencil.size() == width * height);
        assert((int)rgba.size() == width * height);
        uint32_t markerRgba = marker->rgba;

        // === convert to RGBA image ===
        size_t ixMax = stencil.size();
        for (size_t ix = 0; ix < ixMax; ++ix)
            rgba[ix] = stencil[ix] ? markerRgba : 0;
    }

    //* render the RGBA image repeatedly, as defined by the marker sequence */
    static void
    drawRgba2screen(const vector<uint32_t>& rgba, int screenX, int screenY, int screenWidth, int screenHeight) {
        assert(screenWidth * screenHeight == (int)rgba.size());
        Fl_RGB_Image im((const uchar*)&rgba[0], screenWidth, screenHeight, 4);
        im.draw(screenX, screenY);
    }

    /** given limits are extended to include data */
    void updateAutoscaleX(float& x0, float& x1) const {
        if (pDataY) {
            if (pDataX == NULL) {  // xdata exists only combined with Ydata (otherwise, the trade holds just lines)
                x0 = std::min(x0, 1.0f);
                x1 = std::max(x1, (float)(pDataY->size() + 1));
            } else {
                for (float x : *pDataX) {
                    if (!std::isinf(x) && !std::isnan(x)) {
                        x0 = std::min(x0, x);
                        x1 = std::max(x1, x);
                    }
                }
            }
        }
        for (auto x : vertLineX) {
            x0 = std::min(x0, x);
            x1 = std::max(x1, x);
        }
    }

    /** given limits are extended to include data */
    void updateAutoscaleY(float& y0, float& y1) const {
        if (pDataY) {
            for (float y : *pDataY) {
                if (!std::isinf(y) && !std::isnan(y)) {
                    y0 = std::min(y0, y);
                    y1 = std::max(y1, y);
                }
            }
        }
        for (auto y : horLineY) {
            y0 = std::min(y0, y);
            y1 = std::max(y1, y);
        }
    }

    bool findClosestPoint(int xScreen, int yScreen, const proj<float>& p, size_t& ixPt, int& bestDist) const {
        bool r = false;
        if (bestDist == 0)
            return r;  // can't do any better
        if (pDataY) {
            const size_t nPts = pDataY->size();
            if (pDataX) {
                for (size_t ix = 0; ix < nPts; ++ix) {
                    float xData = (*pDataX)[ix];
                    float yData = (*pDataY)[ix];
                    if (xData < p.getDataX0() || (xData > p.getDataX1()) || (yData < p.getDataY0()) || (yData > p.getDataY1()))
                        continue;
                    int xDataP = p.projX(xData);
                    int yDataP = p.projY(yData);
                    int dist = (xDataP - xScreen) * (xDataP - xScreen) + (yDataP - yScreen) * (yDataP - yScreen);
                    if (dist < bestDist) {
                        ixPt = ix;
                        bestDist = dist;
                        r = true;
                    }
                }
            } else {
                for (size_t ix = 0; ix < nPts; ++ix) {
                    float xData = (float)(ix + 1);
                    float yData = (*pDataY)[ix];
                    if (xData < p.getDataX0() || (xData > p.getDataX1()) || (yData < p.getDataY0()) || (yData > p.getDataY1()))
                        continue;
                    int xDataP = p.projX(xData);
                    int yDataP = p.projY(yData);
                    int dist = (xDataP - xScreen) * (xDataP - xScreen) + (yDataP - yScreen) * (yDataP - yScreen);
                    if (dist < bestDist) {
                        ixPt = ix;
                        bestDist = dist;
                        r = true;
                    }
                }
            }
        }
        return r;
    }

    void getPt(size_t ixPt, float& x, float& y) const {
        if (pDataX != NULL)
            x = (*pDataX)[ixPt];
        else
            x = ixPt + 1;
        y = (*pDataY)[ixPt];
    }

    // returns all annotations for a given point
    vector<string> getAnnotations(size_t ixPt) const {
        vector<string> r;
        for (const vector<string>* a : pAnnot) {
            assert(a != NULL);
            if (ixPt < a->size())
                r.push_back((*a)[ixPt]);
        }
        return r;
    }

    const marker_cl* marker;

   protected:
    // X location of points (NULL: use 1, 2, ..., N)
    const vector<float>* pDataX;
    // Y location of points (NULL: no data)
    const vector<float>* pDataY;
    // Annotations, one per pDataY point (trace may have any number of independent annotations)
    const vector<const vector<string>*> pAnnot;
    // vertical lines
    vector<float> vertLineX;
    // horizontal lines
    vector<float> horLineY;
    // mask value for each pDataY point (NULL: no mask). If set, only points with mask==maskVal are plotted.
    const vector<uint16_t>* pMask;
    // mask value (if pMask is non-NULL). If the latter, only points with mask==maskVal are plotted.
    uint16_t maskVal;

    // determines pixel distance squared between xData/yData (data coordinates) and xScreen/yScreen (screen coordinates)
    static int projectedDeltaSquare(const proj<float>& p, float xData, float yData, int xScreen, int yScreen) {
        if (xData < p.getScreenX0() || (xData > p.getScreenX1()) || (yData < p.getScreenY0()) || (yData > p.getScreenY1()))
            return std::numeric_limits<int>::max();
        int xDataP = p.projX(xData);
        int yDataP = p.projY(yData);
        return (xDataP - xScreen) * (xDataP - xScreen) + (yDataP - yScreen) * (yDataP - yScreen);
    }
};  // class drawJob
