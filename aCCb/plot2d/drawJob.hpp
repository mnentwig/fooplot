#pragma once
#include <FL/Fl_Image.H>

#include <cassert>
#include <cmath>  // ceil
#include <future>
#include <string>
#include <vector>

#include "marker.hpp"
#include "proj.hpp"
using std::vector, std::string;
typedef uint8_t stencil_t;  // bool: 32 ms; uint8: 4.5 ms; uint16: 6 ms uint32_t: 9 ms uint64_t: 16 ms
class drawJob {
   protected:
    // multithreaded job description.
    // passed by value - don't put anything large inside
    class job_t {
       public:
        job_t(size_t ixStart, size_t ixEnd, const vector<float>* pDataX, const vector<float>* pDataY, const proj<float> p, vector<stencil_t>* pStencil)
            : ixStart(ixStart),
              ixEnd(ixEnd),
              pDataX(pDataX),
              pDataY(pDataY),
              p(p),
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
        vector<stencil_t>* pStencil;
    };

    static void drawDotsXY(const job_t job) {
        assert(job.pDataX->size() == job.pDataY->size());
        const int width = job.p.getScreenWidth();
        const int height = job.p.getScreenHeight();

        for (size_t ix = job.ixStart; ix < job.ixEnd; ++ix) {
            float plotX = (*(job.pDataX))[ix];
            float plotY = (*(job.pDataY))[ix];
            int pixX = job.p.projX(plotX);
            int pixY = job.p.projY(plotY);
            if ((pixX >= 0) && (pixX < width) && (pixY >= 0) && (pixY < height))
                (*job.pStencil)[pixY * width + pixX] = 1;
        }  // for ix
    }

    static void drawDotsY(const job_t job) {
        const int width = job.p.getScreenWidth();
        const int height = job.p.getScreenHeight();

        for (size_t ix = job.ixStart; ix < job.ixEnd; ++ix) {
            float plotX = ix + 1;
            float plotY = (*(job.pDataY))[ix];
            int pixX = job.p.projX(plotX);
            int pixY = job.p.projY(plotY);
            if ((pixX >= 0) && (pixX < width) && (pixY >= 0) && (pixY < height))
                (*job.pStencil)[pixY * width + pixX] = 1;
        }  // for ix
    }

   public:
    drawJob(const vector<float>* pDataX, const vector<float>* pDataY, const vector<string>* pAnnot, const marker_cl* marker, vector<float> vertLineX, vector<float> horLineY) : marker(marker), pDataX(pDataX), pDataY(pDataY), pAnnot(pAnnot), vertLineX(vertLineX), horLineY(horLineY) {}

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
        vector<job_t> jobs;
        vector<std::future<void>> futs;
        for (size_t chunkIxStart = 0; chunkIxStart < nData; chunkIxStart += chunk) {
            size_t chunkIxEnd = std::min(chunkIxStart + chunk, nData);
            job_t job(chunkIxStart, chunkIxEnd, pDataX, pDataY, p, &stencil);
            jobs.push_back(job);
            if (pDataX)
                futs.push_back(std::async(drawDotsXY, jobs.back()));
            else
                futs.push_back(std::async(drawDotsY, jobs.back()));
        }
        for (std::future<void>& f : futs)
            f.get();
    }

    static vector<stencil_t>
    convolveStencil(const vector<stencil_t>& stencil, int width, int height, const marker_cl* marker) {
        vector<stencil_t> r(stencil);  // center pixel
                                       //        return r;
        int count = 0;
        for (int dx = -marker->dxMinus; dx <= marker->dxPlus; ++dx) {
            for (int dy = -marker->dxMinus; dy <= marker->dxPlus; ++dy) {
                if ((dx == 0) && (dy == 0))
                    continue;  // center pixel is set by return value constructor
                if (marker->seq[count++]) {
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

    void getPt(size_t ixPt, float& x, float& y) {
        if (pDataX != NULL)
            x = (*pDataX)[ixPt];
        else
            x = ixPt + 1;
        y = (*pDataY)[ixPt];
    }

    bool getAnnotation(size_t ixPt, /*out*/ string& a) {
        if (pAnnot == NULL) return false;
        if (ixPt < pAnnot->size()) {
            a = (*pAnnot)[ixPt];
            return true;
        }
        return false;
    }

    const marker_cl* marker;

   protected:
    const vector<float>* pDataX;
    const vector<float>* pDataY;
    const vector<string>* pAnnot;
    vector<float> vertLineX;
    vector<float> horLineY;

    // determines pixel distance squared between xData/yData (data coordinates) and xScreen/yScreen (screen coordinates)
    static int projectedDeltaSquare(const proj<float>& p, float xData, float yData, int xScreen, int yScreen) {
        if (xData < p.getScreenX0() || (xData > p.getScreenX1()) || (yData < p.getScreenY0()) || (yData > p.getScreenY1()))
            return std::numeric_limits<int>::max();
        int xDataP = p.projX(xData);
        int yDataP = p.projY(yData);
        return (xDataP - xScreen) * (xDataP - xScreen) + (yDataP - yScreen) * (yDataP - yScreen);
    }
};  // class drawJob

class allDrawJobs_cl {
   public:
    allDrawJobs_cl() : drawJobs() {}
    void draw(const proj<double>& p) {
        int screenWidth = p.getScreenWidth();
        int screenHeight = p.getScreenHeight();
        int screenX = p.getScreenX0();
        int screenY = p.getScreenY1();

        /* Projection to stencil at x=0 Y=0 */
        const proj<float> projStencil(p.getDataX0(), p.getDataY1(), p.getDataX1(), p.getDataY0(), /*stencil X0*/ 0, /*stencil Y0*/ 0, /*stencil X1*/ screenWidth, /*stencil Y1*/ screenHeight);

        // ... combine subsequent traces with same marker into a  common stencil
        vector<stencil_t> stencil(screenWidth * screenHeight);

        // ... then render the stencil using the marker into rgba
        vector<uint32_t> rgba(screenWidth * screenHeight);

        const marker_cl* currentMarker = NULL;
        for (auto it = drawJobs.begin(); it != drawJobs.end(); ++it) {
            drawJob& j = *it;
            bool stencilHoldsIncompatibleData = (currentMarker != NULL) && (currentMarker != j.marker);
            if (stencilHoldsIncompatibleData) {
                // Draw stencil...
                vector<stencil_t> sConv = drawJob::convolveStencil(stencil, screenWidth, screenHeight, currentMarker);
                drawJob::drawStencil2rgba(sConv, screenWidth, screenHeight, currentMarker, /*out*/ rgba);
                drawJob::drawRgba2screen(rgba, screenX, screenY, screenWidth, screenHeight);
                // ... and clear
                std::fill(stencil.begin(), stencil.end(), 0);
            }  // if incompatible with stencil contents

            j.drawToStencil(projStencil, /*out*/ stencil);
            currentMarker = j.marker;
        }
        // render final stencil
        if (currentMarker != NULL) {
            vector<stencil_t> sConv = drawJob::convolveStencil(stencil, screenWidth, screenHeight, currentMarker);
            drawJob::drawStencil2rgba(sConv, screenWidth, screenHeight, currentMarker, /*out*/ rgba);
            drawJob::drawRgba2screen(rgba, screenX, screenY, screenWidth, screenHeight);
        }
    }

    void addTrace(drawJob j) {
        this->drawJobs.push_back(j);
    }

    void updateAutoscaleX(float& x0, float& x1) const {
        for (auto j : drawJobs)
            j.updateAutoscaleX(x0, x1);
    }

    void updateAutoscaleY(float& y0, float& y1) const {
        for (auto j : drawJobs)
            j.updateAutoscaleY(y0, y1);
    }

    // attempts to locate the on-screen data point closest to xData/yData. Returns true if successful, with trace in ixTrace, point in ixPt.
    bool findClosestPoint(float xData, float yData, const proj<float>& p, size_t& ixTrace, size_t& ixPt) const {
        bool r = false;
        int bestDist = std::numeric_limits<int>::max();
        float xScreen = p.projX(xData);
        float yScreen = p.projY(yData);
        for (size_t ixT = 0; ixT < drawJobs.size(); ++ixT)
            if (drawJobs[ixT].findClosestPoint(xScreen, yScreen, p, ixPt, bestDist)) {
                r = true;
                ixTrace = ixT;
                if (bestDist == 0)
                    break;  // bull's eye
            }
        return r;
    }

    void getPt(size_t ixTrace, size_t ixPt, float& x, float& y) {
        drawJobs[ixTrace].getPt(ixPt, x, y);
    }
    bool getAnnotation(size_t ixTrace, size_t ixPt, /*out*/ string& a) {
        return drawJobs[ixTrace].getAnnotation(ixPt, a);
    }

   protected:
    vector<drawJob>
        drawJobs;
};