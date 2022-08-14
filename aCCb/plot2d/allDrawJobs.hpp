#pragma once
#include <string>
#include <vector>

#include "drawJob.hpp"
#include "marker.hpp"
#include "proj.hpp"
using std::vector, std::string;

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
            if (!j.hasPoints()) {
                // draw lines directly to screen - use of a stencil is inefficient (unless we need it anyway for data)
                // matters when lines of different colors are used in many plots that show up at the same time
                j.drawLinesDirectly(p);
            } else {
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
        }
        // render final stencil
        if (currentMarker != NULL) {
            vector<stencil_t> sConv = drawJob::convolveStencil(stencil, screenWidth, screenHeight, currentMarker);
            drawJob::drawStencil2rgba(sConv, screenWidth, screenHeight, currentMarker, /*out*/ rgba);
            drawJob::drawRgba2screen(rgba, screenX, screenY, screenWidth, screenHeight);
        }
    }

    // adds a new drawJob
    void addDrawJob(drawJob j) {
        this->drawJobs.push_back(j);
    }

    // extends x0, x1 to include x range
    void updateAutoscaleX(float& x0, float& x1) const {
        for (auto j : drawJobs)
            j.updateAutoscaleX(x0, x1);
    }

    // extends y0, y1 to include y range
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
        assert(ixTrace < drawJobs.size());
        drawJobs[ixTrace].getPt(ixPt, x, y);
    }

    // returns all annotations for a given point on a given trace
    vector<string> getAnnotations(size_t ixTrace, size_t ixPt) const {
        assert(ixTrace < drawJobs.size());
        return drawJobs[ixTrace].getAnnotations(ixPt);
    }

   protected:
    vector<drawJob> drawJobs;
};