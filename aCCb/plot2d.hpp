#pragma once
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

#include <algorithm>  // sort
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>  // isinf
#include <future>
#include <iostream>
#include <limits>  // inf
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "plot2d/allDrawJobs.hpp"
#include "plot2d/annotator.hpp"
#include "plot2d/axisTics.hpp"
#include "plot2d/drawJob.hpp"
#include "plot2d/marker.hpp"
#include "plot2d/proj.hpp"
#include "vectorText.hpp"
#include "widget.hpp"

namespace aCCb {
using std::vector, std::string, std::array, std::unordered_set, std::cout, std::endl;
class plot2d : public Fl_Box {
    class evtMan_cl {
       public:
        evtMan_cl(plot2d* parent) : parent(parent){};
        double mouseDown1DataX;
        double mouseDown1DataY;
        double mouseDown3DataX;
        double mouseDown3DataY;
        plot2d* parent;
        double drawRectx0;
        double drawRecty0;
        double drawRectx1;
        double drawRecty1;
        class mouseState_cl {
           public:
            mouseState_cl() : state(0) {}
            void update(int event) {
                // === track mouse buttons ===
                int stateUpdate = Fl::event_buttons();
                stateDeltaOn = stateUpdate & ~state;
                stateDeltaOff = state & ~stateUpdate;
                state = stateUpdate;

                switch (event) {
                    case FL_PUSH:
                    case FL_RELEASE:
                    case FL_DRAG:
                    case FL_MOUSEWHEEL:
                    case FL_MOVE:
                        int mx = Fl::event_x();
                        int my = Fl::event_y();
                        if ((mx != mouseX) || (my != mouseY)) {
                            mouseX = mx;
                            mouseY = my;
                            mouseMoved = true;
                        } else
                            mouseMoved = false;
                }  // switch

                if (event == FL_MOUSEWHEEL)
                    mouseWheel = Fl::event_dy();
                else
                    mouseWheel = 0;
            }
            bool getDeltaOn(int mask) {
                return (stateDeltaOn & mask) != 0;
            }
            bool getDeltaOff(int mask) {
                return (stateDeltaOff & mask) != 0;
            }
            bool getState(int mask) {
                return (state & mask) != 0;
            }
            bool getMouseMove() {
                return mouseMoved;
            }
            int getMouseWheel() {
                return mouseWheel;
            }
            int getMouseX() {
                return mouseX;
            }
            int getMouseY() {
                return mouseY;
            }

           protected:
            int state;
            int stateDeltaOn;
            int stateDeltaOff;
            int mouseX;
            int mouseY;
            bool mouseMoved;
            int mouseWheel;
        } mouseState;

        int handle(int event) {
            switch (event) {
                case FL_FOCUS:    // must ack to receive keypress events
                case FL_UNFOCUS:  // must ack to receive keypress events
                case FL_ENTER:
                    return 1;  // must ack to receive mouse events
                case FL_PUSH:
                case FL_RELEASE:
                case FL_MOUSEWHEEL:
                case FL_DRAG:
                case FL_MOVE:
                    break;
                case FL_KEYDOWN:
                    return handleKeyDown(Fl::event_key());
                default:
                    return 0;
            }
            proj<double> p = parent->projDataToScreen<double>();
            mouseState.update(event);
            int mouseX = mouseState.getMouseX();
            int mouseY = mouseState.getMouseY();
            double dataX = p.unprojX(mouseX);
            double dataY = p.unprojY(mouseY);
            if (mouseState.getDeltaOn(FL_BUTTON1)) {
                mouseDown1DataX = dataX;
                mouseDown1DataY = dataY;
            }
            if (mouseState.getDeltaOn(FL_BUTTON3)) {
                mouseDown3DataX = dataX;
                mouseDown3DataY = dataY;
                drawRectx0 = dataX;
                drawRecty0 = dataY;
                drawRectx1 = dataX;
                drawRecty1 = dataY;
            }
            if (mouseState.getDeltaOff(FL_BUTTON3))
                if ((std::fabs(mouseDown3DataX - dataX) > 1e-18) || (std::fabs(mouseDown3DataY - dataY) > 1e-18))
                    parent->setViewArea(/*x0*/ std::min(mouseDown3DataX, dataX), /*y0*/ std::min(mouseDown3DataY, dataY), /*x1*/ std::max(mouseDown3DataX, dataX), /*y1*/ std::max(mouseDown3DataY, dataY), /*resetAxes*/ true);

            if (mouseState.getMouseMove()) {
                parent->notifyCursorMove(dataX, dataY);
                if (mouseState.getState(FL_BUTTON1)) {
                    double dx = dataX - mouseDown1DataX;
                    double dy = dataY - mouseDown1DataY;
                    double x0, y0, x1, y1;
                    parent->getViewArea(x0, y0, x1, y1);
                    x0 -= dx;
                    x1 -= dx;
                    y0 -= dy;
                    y1 -= dy;
                    dataX -= dx;
                    dataY -= dy;
                    mouseDown1DataX = dataX;
                    mouseDown1DataY = dataY;
                    parent->setViewArea(x0, y0, x1, y1, /*resetAxes*/ false);
                }
                if (mouseState.getState(FL_BUTTON3)) {
                    drawRectx1 = dataX;
                    drawRecty1 = dataY;
                    parent->cursorRedraw();
                }
            }

            int d = mouseState.getMouseWheel();
            if (d != 0) {
                double scale = 1.3;
                if (d < 0)
                    scale = 1 / scale;

                bool scaleYflag = !Fl::event_state(FL_SHIFT);
                bool scaleXflag = !Fl::event_state(FL_CTRL);

                double x0, y0, x1, y1;
                parent->getViewArea(x0, y0, x1, y1);
                if (scaleXflag) {
                    double deltaX0 = x0 - dataX;
                    double deltaX1 = x1 - dataX;
                    deltaX0 *= scale;
                    deltaX1 *= scale;
                    x0 = deltaX0 + dataX;
                    x1 = deltaX1 + dataX;
                }
                if (scaleYflag) {
                    double deltaY0 = y0 - dataY;
                    double deltaY1 = y1 - dataY;
                    deltaY0 *= scale;
                    deltaY1 *= scale;
                    y0 = deltaY0 + dataY;
                    y1 = deltaY1 + dataY;
                }
                parent->setViewArea(x0, y0, x1, y1, /*resetAxes*/ true);
            }
            return 1;
        }

        int handleKeyDown(int key) {
            switch (key) {
                case 'a':
                    if (Fl::event_state(FL_SHIFT))
                        parent->autoscaleX(true, true);
                    else if (Fl::event_state(FL_CTRL))
                        parent->autoscaleY(true, true);
                    else {
                        parent->autoscaleX(true, true);
                        parent->autoscaleY(true, true);
                    }
                    parent->invalidate(/*full redraw*/ true);
                    return true;
                case 'm':
                    parent->toggleCursor();
                    return true;
            }
            return false;
        }

        bool drawRect(double& x0, double& y0, double& x1, double& y1) {
            if (mouseState.getState(FL_BUTTON3)) {
                x0 = drawRectx0;
                y0 = drawRecty0;
                x1 = drawRectx1;
                y1 = drawRecty1;
                return true;
            } else
                return false;
        }
    } evtMan;

    //* fltk event handler */
    int handle(int event) {
        return evtMan.handle(event);
    }

    /** gets the visible plot area */
    void getViewArea(double& x0, double& y0, double& x1, double& y1) const {
        x0 = this->x0;
        y0 = this->y0;
        x1 = this->x1;
        y1 = this->y1;
    }

    //* sets the visible area, triggers redraw */
    void setViewArea(double x0, double y0, double x1, double y1, bool resetAxes) {
        // better safe than sorry
        double x0tmp = std::min(x0, x1);
        double y0tmp = std::min(y0, y1);
        double x1tmp = std::max(x0, x1);
        double y1tmp = std::max(y0, y1);

        // === enforce minimum span ===
        // (approx. limit of float precision in 1920x1080 resolution)
        const double minSpan = 5e-12;
        if ((x1tmp - x0tmp) < minSpan) {
            double avg = (x0tmp + x1tmp) / 2.0;
            x0tmp = avg - minSpan / 2.0;
            x1tmp = avg + minSpan / 2.0;
        }
        if ((y1tmp - y0tmp) < minSpan) {
            double avg = (y0tmp + y1tmp) / 2.0;
            y0tmp = avg - minSpan / 2.0;
            y1tmp = avg + minSpan / 2.0;
        }

        this->x0 = x0tmp;
        this->y0 = y0tmp;
        this->x1 = x1tmp;
        this->y1 = y1tmp;
        needFullRedraw = true;
        if (resetAxes) {
            // reset the state of currently drawn axis tic labels e.g. on zoom change (re-initialize overlap suppression)
            drawnAxisTicQuantX.clear();
            drawnAxisTicQuantY.clear();
        }
        redraw();
    }
    void cursorRedraw() {
        redraw();
    }

    template <typename T>
    proj<T> projDataToScreen() {
        int axisMarginTop = (title != "" ? titleFontsize : 0);
        int axisMarginLeft = fontsize + (ylabel != "" ? axisLabelFontsize : 0);
        int axisMarginBottom = fontsize + (xlabel != "" ? axisLabelFontsize : 0);
        // bottom left
        int screenX0 = x() + axisMarginLeft;
        int screenY0 = y() + h() - axisMarginBottom;
        // top right
        int screenX1 = x() + w();
        int screenY1 = y() + axisMarginTop;
        return proj<T>(x0, y0, x1, y1, screenX0, screenY0, screenX1, screenY1);
    }

    void drawTitle(const proj<double>& p) const {
        vector<array<float, 4>> geom = aCCb::vectorFont::renderText(title);
        geom = aCCb::vectorFont::centerX(geom);
        aCCb::vectorFont::bbox b(geom);
        aCCbWidget::renderText(geom, titleFontsize, p.getScreenXCenter() - b.getWidth(), p.getScreenY1() - titleFontsize);
    }

    void drawXlabel(const proj<double>& p) const {
        vector<array<float, 4>> geom = aCCb::vectorFont::renderText(xlabel);
        geom = aCCb::vectorFont::centerX(geom);
        aCCb::vectorFont::bbox b(geom);
        aCCbWidget::renderText(geom, axisLabelFontsize, p.getScreenXCenter() - b.getWidth(), p.getScreenY0() + fontsize);
    }

    void drawYlabel(const proj<double>& p) const {
        vector<array<float, 4>> geom = aCCb::vectorFont::renderText(ylabel);
        aCCb::vectorFont::rotate270(geom);
        geom = aCCb::vectorFont::centerY(geom);
        aCCb::vectorFont::bbox b(geom);
        aCCbWidget::renderText(geom, axisLabelFontsize, p.getScreenX0() - fontsize, p.getScreenYCenter() - b.getHeight());
    }

   public:
    plot2d(int x, int y, int w, int h, allDrawJobs_cl& adr)
        : Fl_Box(x, y, w, h), evtMan(this), allDrawJobs(adr), annotator(adr) {}
    ~plot2d() {}
    void shutdown() {
        annotator.shutdown();
    }

    void setTitle(const string& v) {
        title = v;
    }

    void setXlabel(const string& v) {
        xlabel = v;
    }

    void setYlabel(const string& v) {
        ylabel = v;
    }

    // autoscales given limits. Returns 1 if input was meaningful
    bool autoscaleX(bool scaleX0, bool scaleX1) {
        if (false == (scaleX0 | scaleX1))
            return true;  // nothing to do

        const double inf = std::numeric_limits<float>::infinity();
        // autoscale: extend inversed range to fit each item
        float x0f = inf;
        float x1f = -inf;

        allDrawJobs.updateAutoscaleX(x0f, x1f);

        // convert infinity to "large" number
        const float INFTY = 1e16;  // fallback: "10 tera"
        if (std::isinf(x0f)) x0f = -INFTY;
        if (std::isinf(x1f)) x1f = INFTY;

        // apply result, if meaningful
        const float eps = 1e-16;  // fallback: "0.1 femto"
        // check for non-zero span
        bool validX = (scaleX1 ? x1f : x1) - (scaleX0 ? x0f : x0) > eps;
        if (scaleX0 && validX) x0 = x0f;
        if (scaleX1 && validX) x1 = x1f;

        // === add margin to autoscale range ===
        double gap = x1 - x0;
        if (scaleX0 && validX) x0 -= gap * autoscaleMarginOneSided;
        if (scaleX1 && validX) x1 += gap * autoscaleMarginOneSided;

        // check success
        if (!validX)
            if (scaleX0 or scaleX1) return false;  // unable to scale X
        return true;
    }

    // autoscales given limits. Returns 1 if input was meaningful
    bool autoscaleY(bool scaleY0, bool scaleY1) {
        if (false == (scaleY0 | scaleY1))
            return true;  // nothing to do

        const double inf = std::numeric_limits<float>::infinity();
        // autoscale: extend inversed range to fit each item
        float y0f = inf;
        float y1f = -inf;

        allDrawJobs.updateAutoscaleY(y0f, y1f);

        // convert infinity to "large" number
        const float INFTY = 1e16;  // fallback: "10 tera"
        if (std::isinf(y0f)) y0f = -INFTY;
        if (std::isinf(y1f)) y1f = INFTY;

        // apply result, if meaningful
        const float eps = 1e-16;  // fallback: "0.1 femto"
        bool validY = (scaleY1 ? y1f : y1) - (scaleY0 ? y0f : y0) > eps;
        if (scaleY0 && validY) y0 = y0f;
        if (scaleY1 && validY) y1 = y1f;

        // === add margin to autoscale range ===
        double gap = y1 - y0;
        if (scaleY0 && validY) y0 -= gap * autoscaleMarginOneSided;
        if (scaleY1 && validY) y1 += gap * autoscaleMarginOneSided;

        // check success
        if (!validY)
            if (scaleY0 or scaleY1) return false;  // unable to scale Y
        return true;
    }

    // get regular callbacks for non-blocking background work
    void cb_timer() {
        size_t ixTr;
        size_t ixPt;
        if (annotator.getHighlightedPoint(ixTr, ixPt))
            if (this->cursorHighlight.setHighlight(ixTr, ixPt))
                redraw();  // redraw on change (reuse bitmap)
    }

    void invalidate(bool needFullRedraw) {
        this->needFullRedraw = needFullRedraw;
        redraw();
    }

    void toggleCursor() {
        cursorFlag = !cursorFlag;
        invalidate(/*don't need full redraw*/ false);
    }

   protected:
    // helper class: drawing instructions for a tic label. Purpose is to consistently suppress drawing of less-important labels in case of overlap.
    class ticLabel {
       public:
        ticLabel(vector<array<float, 4>>& geom, aCCb::vectorFont::bbox bbox, size_t decimationLevel, int64_t quant) : geom(geom), bbox(bbox), decimationLevel(decimationLevel), quant(quant) {}

        // text to draw
        vector<array<float, 4>> geom;
        // bounding box of drawn text
        aCCb::vectorFont::bbox bbox;
        // importance (drawing order) - the higher, the more important
        size_t decimationLevel;
        // multiple of resolution step
        int64_t quant;

        static void drawTicLabels(vector<ticLabel>& ticLabels, vector<int64_t>& lastDrawnQuant) {
            // copy previously drawn quantization tics
            const vector<int64_t> lastDrawnQuantPrev(lastDrawnQuant);
            // reset list to be updated
            lastDrawnQuant.clear();

            // order ticLabels, most important ones first
            std::sort(
                ticLabels.begin(), ticLabels.end(),
                [](ticLabel& a, ticLabel& b) -> bool { return a.decimationLevel >= b.decimationLevel; });

            vector<const aCCb::vectorFont::bbox*> bboxesDrawn;

            // === loop over blocks with a common decimation level ===
            size_t ixDrawOuter = 0;
            while (ixDrawOuter < ticLabels.size()) {
                size_t currentDecimationLevel = ticLabels[ixDrawOuter].decimationLevel;

                size_t ixStart = ixDrawOuter;
                for (size_t pass = 0; pass <= 1; ++pass) {
                    for (size_t ixDraw = ixStart; ixDraw < ticLabels.size(); ++ixDraw) {
                        const ticLabel& tl = ticLabels[ixDraw];

                        // === check change in decimation level ===
                        if (tl.decimationLevel != currentDecimationLevel)
                            break;

                        ixDrawOuter = ixDraw + 1;  // update first undrawn element for outer loop

                        bool ticWasDrawnTheLastTime = std::find(lastDrawnQuantPrev.begin(), lastDrawnQuantPrev.end(), tl.quant) != lastDrawnQuantPrev.end();
                        // first pass: Try tics that were drawn the last time
                        if ((pass == 0) && !ticWasDrawnTheLastTime) continue;
                        // second pass: Try tics that were not drawn the last time
                        if ((pass == 1) && ticWasDrawnTheLastTime) continue;

                        // === check for collision with anything drawn before on this axis ===
                        bool collision = false;
                        for (const aCCb::vectorFont::bbox* drawnBbox : bboxesDrawn) {
                            if (drawnBbox->overlaps(tl.bbox)) {
                                collision = true;
                                break;
                            }
                        }
                        if (!collision) {
                            bboxesDrawn.push_back(&tl.bbox);  // add this label to collision check
                            aCCbWidget::renderText(tl.geom);  // draw
                            lastDrawnQuant.push_back(tl.quant);
                        }

                    }  // for ixDraw
                }      // for pass
            }
        }  // drawTicLabels
    };     // class ticLabel

    // draws title, labels, axes
    void drawPlotDecorations(const proj<double>& p) {
        fl_push_clip(x(), y(), w(), h());  // clip to widget area

        drawTitle(p);
        drawXlabel(p);
        drawYlabel(p);

        vector<double> xAxisDeltas = axisTics::getTicDelta(x0, x1);
        double xAxisDeltaMajor = xAxisDeltas[0];
        double xAxisDeltaMinor = xAxisDeltas[1];

        const vector<axisTics::ticVal> xAxisTicsMajor = axisTics::getTicVals(x0, x1, xAxisDeltaMajor);
        const vector<axisTics::ticVal> xAxisTicsMinor = axisTics::getTicVals(x0, x1, xAxisDeltaMinor);
        // === draw axes ===
        aCCbWidget::line(
            p.projX(x0), p.projY(y1),   // top left
            p.projX(x0), p.projY(y0),   // bottom left
            p.projX(x1), p.projY(y0));  // bottom right

        // === draw x axis minor tics ===
        for (const axisTics::ticVal& ticX : xAxisTicsMinor)  // draw minor tics before data
            aCCbWidget::line(p.projX(ticX.val), p.projY(y0), p.projX(ticX.val), p.projY(y0) - minorTicLength);

        // === draw x axis major tics and numbers ===
        vector<ticLabel> ticLabelsX;
        vector<string> xAxisTicsMajorStr = axisTics::formatTicVals(xAxisTicsMajor, xAxisDeltaMajor);
        for (size_t ix = 0; ix < xAxisTicsMajor.size(); ++ix) {  // todo draw major tics after data
            const axisTics::ticVal& ticX = xAxisTicsMajor[ix];

            // === long line across the plot ===
            fl_color(FL_DARK_GREEN);
            aCCbWidget::line(p.projX(ticX.val), p.projY(y0), p.projX(ticX.val), p.projY(y1));

            // === short tic line ===
            fl_color(FL_GREEN);
            aCCbWidget::line(p.projX(ticX.val), p.projY(y0), p.projX(ticX.val), p.projY(y0) - majorTicLength);

            // === tic label ===
            string ticStr = xAxisTicsMajorStr[ix];
            vector<array<float, 4>> geom = aCCb::vectorFont::renderText(ticStr.c_str());
            geom = aCCb::vectorFont::centerX(geom);
            int originX = p.projX(ticX.val);
            int originY = p.projY(y0);
            geom = aCCb::vectorFont::project(geom, fontsize, originX, originY);

            // === tic label text ===
            // get label text bounding box
            aCCb::vectorFont::bbox b(geom);
            // enforce some "space" between tic labels
            b.extend(/*dx*/ fontsize, /*dy*/ 0);

            ticLabelsX.emplace_back(geom, b, ticX.decimationLevel, ticX.quant);
        }

        // === draw xlabels ===
        ticLabel::drawTicLabels(ticLabelsX, drawnAxisTicQuantX);

        vector<double> yAxisDeltas = axisTics::getTicDelta(y0, y1);
        double yAxisDeltaMajor = yAxisDeltas[0];
        double yAxisDeltaMinor = yAxisDeltas[1];

        const vector<axisTics::ticVal> yAxisTicsMajor = axisTics::getTicVals(y0, y1, yAxisDeltaMajor);
        const vector<axisTics::ticVal> yAxisTicsMinor = axisTics::getTicVals(y0, y1, yAxisDeltaMinor);

        // === draw y axis minor tics ===
        for (const axisTics::ticVal& ticY : yAxisTicsMinor)  // draw minor tics before data
            aCCbWidget::line(p.projX(x0), p.projY(ticY.val), p.projX(x0) + minorTicLength, p.projY(ticY.val));

        // === draw x axis major tics and numbers ===
        vector<string> yAxisTicsMajorStr = axisTics::formatTicVals(yAxisTicsMajor, yAxisDeltaMajor);
        vector<ticLabel> ticLabelsY;
        for (size_t ix = 0; ix < yAxisTicsMajor.size(); ++ix) {  // todo draw major tics after data
            const axisTics::ticVal& ticY = yAxisTicsMajor[ix];

            // === long line across the plot ===
            fl_color(FL_DARK_GREEN);
            aCCbWidget::line(p.projX(x0), p.projY(ticY.val), p.projX(x1), p.projY(ticY.val));

            // === short tic line ===
            fl_color(FL_GREEN);
            aCCbWidget::line(p.projX(x0), p.projY(ticY.val), p.projX(x0) + majorTicLength, p.projY(ticY.val));

            // === tic label ===
            string ticStr = yAxisTicsMajorStr[ix];
            vector<array<float, 4>> geom = aCCb::vectorFont::renderText(ticStr.c_str());
            aCCb::vectorFont::rotate270(geom);
            geom = aCCb::vectorFont::centerY(geom);

            int originX = p.projX(x0);
            int originY = p.projY(ticY.val);
            geom = aCCb::vectorFont::project(geom, fontsize, originX, originY);

            // === tic label text ===
            // get label text bounding box
            aCCb::vectorFont::bbox b(geom);
            // enforce some "space" between tic labels
            b.extend(/*dx*/ 0, /*dy*/ fontsize);

            ticLabelsY.emplace_back(geom, b, ticY.decimationLevel, ticY.quant);
        }

        // === draw ylabels ===
        ticLabel::drawTicLabels(ticLabelsY, drawnAxisTicQuantY);

        fl_pop_clip();
    }

    //* captures image for fast redraw where permitted e.g. when drawing zoom box */
    class cachedImage_cl {
       public:
        cachedImage_cl() : data(NULL), lastW(0), lastH(0) {}
        void capture(int x, int y, int w, int h) {
            if ((w != lastW) || (h != lastH)) {
                delete[] data;  // NULL is OK
                data = NULL;
            }
            lastW = w;
            lastH = h;
            data = fl_read_image(data, x, y, w, h);
        }
        bool restore(int x, int y, int w, int h) const {
            if ((w != lastW) || (h != lastH))
                return false;
            Fl_RGB_Image im((const uchar*)&data[0], w, h, 3);
            im.draw(x, y);
            return true;
        }
        ~cachedImage_cl() {
            delete[] data;
        }

       protected:
        uchar* data;
        int lastW;
        int lastH;
    } cachedImage;

    void draw() {
        this->Fl_Box::draw();

        proj<double> p = projDataToScreen<double>();
        const int screenX = p.getScreenX0();
        const int screenY = p.getScreenY1();
        const int width = p.getScreenWidth();
        const int height = p.getScreenHeight();

        // === try to reload the cached bitmap ===
        if (!this->needFullRedraw)
            if (cachedImage.restore(x(), y(), w(), h()))
                goto skipDataDrawing;

        // === background ===
        fl_rectf(x(), y(), w(), h(), FL_BLACK);

        fl_line_style(FL_SOLID);
        fl_color(FL_GREEN);

        {
            // auto begin = std::chrono::high_resolution_clock::now();
            this->drawPlotDecorations(p);
            // auto end = std::chrono::high_resolution_clock::now();
            // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            // cout << "axes:\t" << 1e-6 * (double)duration << " ms" << endl;
        }

        // === plot ===
        fl_push_clip(screenX, screenY, width, height);
        {
            // auto begin = std::chrono::high_resolution_clock::now();

            allDrawJobs.draw(p);
            // auto end = std::chrono::high_resolution_clock::now();
            // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            // cout << "drawJobs:\t" << 1e-6 * (double)duration << " ms" << endl;
        }
        fl_pop_clip();

        // === save whole drawing area image for cursor operations ===
        // todo include axes
        {
            // auto begin = std::chrono::high_resolution_clock::now();

            cachedImage.capture(x(), y(), w(), h());
            // auto end = std::chrono::high_resolution_clock::now();
            // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            // cout << "cache:\t" << 1e-6 * (double)duration << " ms" << endl;
        }

        needFullRedraw = false;
    skipDataDrawing:
        // === draw zoom rectangle ===
        fl_color(FL_WHITE);
        proj<double> pd = projDataToScreen<double>();
        double xr0, yr0, xr1, yr1;
        if (evtMan.drawRect(xr0, yr0, xr1, yr1)) {
            int xs0 = pd.projX(xr0);
            int ys0 = pd.projY(yr0);
            int xs1 = pd.projX(xr1);
            int ys1 = pd.projY(yr1);
            fl_rect(std::min(xs0, xs1), std::min(ys0, ys1), std::abs(xs1 - xs0), std::abs(ys1 - ys0));
        }

        // === draw highlighted point ===
        if (cursorFlag && cursorHighlight.highlightValid) {
            fl_push_clip(screenX, screenY, width, height);
            float x, y;
            allDrawJobs.getPt(cursorHighlight.highlightIxTrace, cursorHighlight.highlightIxPt, x, y);
            int xs = pd.projX(x);
            int ys = pd.projY(y);
            const int d = 10;
            fl_color(FL_RED);
            fl_line(xs - d, ys - d, xs + d, ys + d);
            fl_color(FL_BLUE);
            fl_line(xs - d, ys + d, xs + d, ys - d);
            fl_pop_clip();
        }

        // === draw annotation ===
        if (cursorFlag && (cursorHighlight.annot.size() > 0)) {
            fl_color(FL_GREEN);
            int x = p.getScreenX0() + fontsize / 2;
            int y = p.getScreenY0() - cursorHighlight.annot.size() * fontsize - fontsize / 2;
            for (size_t ix = 0; ix < cursorHighlight.annot.size(); ++ix) {
                vector<array<float, 4>> geom = aCCb::vectorFont::renderText(cursorHighlight.annot[ix].c_str());
                // create bold black background
                fl_color(FL_BLACK);
                aCCbWidget::renderText(geom, fontsize, x - 1, y - 1);
                aCCbWidget::renderText(geom, fontsize, x - 1, y + 1);
                aCCbWidget::renderText(geom, fontsize, x + 1, y - 1);
                aCCbWidget::renderText(geom, fontsize, x + 1, y + 1);
                // draw text in green
                fl_color(FL_GREEN);
                aCCbWidget::renderText(geom, fontsize, x, y);
                y += fontsize;
            }
        }
    }

    // event manager calls this for cursor, annotation search update
    void notifyCursorMove(double dataX, double dataY) {
        proj<float> p = projDataToScreen<float>();
        annotator.notifyCursorChange(dataX, dataY, p);
        cursorHighlight.notifyCursorChange(dataX, dataY, allDrawJobs);
    }

   public:
    /** visible area */
    double x0 = -1;
    double x1 = 2;
    double y0 = 1.23;
    double y1 = 1.24;

    float fontsize = 14;
    float titleFontsize = 18;
    float axisLabelFontsize = fontsize;

   protected:
    class cursorHighlight_t {
       public:
        // set new point to highlight. Returns true if changed (redraw required)
        bool setHighlight(size_t highlightIxTrace, size_t highlightIxPt) {
            bool r = (this->highlightIxTrace != highlightIxTrace) || (this->highlightIxPt |= highlightIxPt);
            this->highlightIxTrace = highlightIxTrace;
            this->highlightIxPt = highlightIxPt;
            this->highlightValid = true;
            return r;
        }

        void notifyCursorChange(double cursorX, double cursorY, allDrawJobs_cl& adj) {
            this->cursorX = cursorX;
            this->cursorY = cursorY;

            annot.clear();
            std::stringstream ss;
            ss << "cur: [" << cursorX << ", " << cursorY << "]";
            annot.push_back(ss.str().c_str());

            if (highlightValid) {
                float x, y;
                adj.getPt(highlightIxTrace, highlightIxPt, x, y);
                ss = std::stringstream();
                ss << " pt:[" << x << ", " << y << "] ";
                annot.push_back(ss.str());

                vector<string> allAnnot = adj.getAnnotations(highlightIxTrace, highlightIxPt);
                for (const string& oneAnnot : allAnnot)
                    annot.push_back(oneAnnot);
            }
        }

        double cursorX = std::numeric_limits<double>::quiet_NaN();
        double cursorY = std::numeric_limits<double>::quiet_NaN();
        size_t highlightIxTrace;
        size_t highlightIxPt;
        bool highlightValid = false;
        vector<string> annot;
    } cursorHighlight;

    allDrawJobs_cl& allDrawJobs;
    annotator_t annotator;

    const int minorTicLength = 3;
    const int majorTicLength = 7;
    bool cursorFlag = false;

    //* if autoscaling, add this margin on each side so the outermost point doesn't fall onto the border */
    const double autoscaleMarginOneSided = 0.03;

    //* if false, use cached bitmap. Otherwise redraw from data. */
    bool needFullRedraw = true;

    //* title displayed on top of the plot */
    string title = "";
    //* x axis label displayed at the bottom */
    string xlabel = "";
    //* y axis label displayed on the left */
    string ylabel = "";

    // grid-quantized values of last drawn x axis tic labels
    vector<int64_t> drawnAxisTicQuantX;

    // grid-quantized values of last drawn y axis tic labels
    vector<int64_t> drawnAxisTicQuantY;
};
}  // namespace aCCb