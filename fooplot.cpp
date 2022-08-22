#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <filesystem>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "aCCb/binIo.hpp"
#include "aCCb/cmdLineParsing.hpp"
#include "aCCb/plot2d.hpp"
#include "aCCb/plot2d/syncFile.hpp"
#include "aCCb/stringToNum.hpp"
#include "aCCb/stringUtil.hpp"
#include "aCCb/vectorText.hpp"
#include "aCCb/widget.hpp"
#include "fooplot/cmdLineProcessor.hpp"
#include "fooplot/markerMan.hpp"
#include "fooplot/testcase.hpp"
#include "fooplot/traceMan.hpp"
using std::string, std::vector, std::array, std::cout, std::endl, std::runtime_error, std::map, std::pair, std::cerr;

#if 0
class myMenu : public aCCbWidget {
   public:
    myMenu(int x, int y, int w, int h) : aCCbWidget(x, y, w, h) {
        this->labelcolor(FL_GREEN);  // propagates to widgets
        cur = cursor(x, y, /*column width*/ 100, /*row height*/ 20);
        this->cb1 = create("log", 50, 0);
        this->cb2 = create("log", 50, 0);
        this->cb3 = create("log", X0);
        cur.move(DYL);  // space for label on top
        this->i1 = create("my text", DX);
        this->i2 = create("my text", X0);
        cur.move(DYL);  // space for label on top
        this->fi = create("my float", X0);
        // Fl_Button *b =
        (Fl_Button *)create("Click me!", DY);
    }
    void callbackButton(Fl_Button *) {
    }
    void callbackCheckbutton(Fl_Check_Button *) {
    }
    void callbackInput(Fl_Input *) {
        std::cout << "inp" << std::endl;
    }

   protected:
    Fl_Button *bPickFile;
    Fl_Check_Button *cb1;
    Fl_Check_Button *cb2;
    Fl_Check_Button *cb3;
    Fl_Input *i1;
    Fl_Input *i2;
    Fl_Float_Input *fi;
};
#endif
class myTestWin {
   public:
    myTestWin(fooplotCmdLineArgRoot &l, allDrawJobs_cl &adr) : syncfile(l.syncfile), persistfile(l.persistfile) {
        // === main window ===
        int areaW, areaH;
        if ((l.windowX > 0) && (l.windowY > 0) && (l.windowW > 0) && (l.windowH > 0)) {
            window = new Fl_Double_Window(l.windowX, l.windowY, l.windowW, l.windowH);
            areaW = l.windowW;
            areaH = l.windowH;
        } else if ((l.windowW > 0) && (l.windowH > 0)) {
            window = new Fl_Double_Window(l.windowX, l.windowY);
            areaW = l.windowW;
            areaH = l.windowH;
        } else {
            int dim = std::min(Fl::w(), Fl::h());
            window = new Fl_Double_Window(dim / 2, dim / 2);
            areaW = dim / 2;
            areaH = dim / 2;
        }
        window->size(areaW, areaH);  // constructor arg is unreliable
        window->callback(cb_closeWrapper, (void *)this);
        window->color(FL_BLACK);

        // === plot area ===
        tb = new aCCb::plot2d(0, 0, areaW, areaH, adr);
        if (!std::isnan(l.xLimLow))
            tb->x0 = l.xLimLow;
        if (!std::isnan(l.xLimHigh))
            tb->x1 = l.xLimHigh;
        if (!std::isnan(l.yLimLow))
            tb->y0 = l.yLimLow;
        if (!std::isnan(l.yLimHigh))
            tb->y1 = l.yLimHigh;

        tb->setTitle(l.title);
        tb->fontsize = l.fontsize;
        tb->axisLabelFontsize = l.fontsize;
        tb->titleFontsize = l.fontsize * 1.5;
        window->label(l.title.c_str());
        tb->setXlabel(l.xlabel);
        tb->setYlabel(l.ylabel);

        bool needAutoscaleX0 = std::isnan(l.xLimLow);
        bool needAutoscaleX1 = std::isnan(l.xLimHigh);
        if (needAutoscaleX0 | needAutoscaleX1) {
            bool success = tb->autoscaleX(needAutoscaleX0, needAutoscaleX1);
            if (!success)
                tb->autoscaleX(true, true);
        }

        bool needAutoscaleY0 = std::isnan(l.yLimLow);
        bool needAutoscaleY1 = std::isnan(l.yLimHigh);
        if (needAutoscaleY0 | needAutoscaleY1) {
            bool success = tb->autoscaleY(needAutoscaleY0, needAutoscaleY1);
            if (!success)
                tb->autoscaleY(true, true);
        }

        // this->menu = new myMenu(800, 0, 400, 200);
        // menu->box(Fl_Boxtype::FL_BORDER_FRAME);
        // menu->color(FL_GREEN);
        window->resizable(tb);
        window->end();
        Fl::add_timeout(0.1, cb_timerWrapper, (void *)this);
    }

    void show() {
        window->show();
    }

    void cb_close() {
        window->hide();
    }

    void shutdown() {
        // need to stop background processes, before removal of the window triggers destructors
        Fl::remove_timeout(cb_timerWrapper);
        tb->shutdown();
    }

    static void cb_closeWrapper(Fl_Widget *w, void *userdata) {
        myTestWin *this_ = (myTestWin *)userdata;
        this_->cb_close();
    }

    void cb_timer() {
        if (syncfile.isModified())
            cb_close();  // all windows are hidden => Fl::run() returns
        else
            Fl::repeat_timeout(0.5, cb_timerWrapper, (void *)this);

        if (persistfile != "") {
            std::stringstream ss;
            ss << "-windowX " << window->x()
               << " -windowY " << window->y()
               << " -windowW " << std::max(window->w(), 300)
               << " -windowH " << std::max(window->h(), 300);
            const string newContents = ss.str();
            if (newContents != persistFileContents) {
                persistFileContents = newContents;
                std::ofstream s(persistfile);
                s << persistFileContents;
            }
        }
    }

    static void cb_timerWrapper(void *userdata) {
        assert(userdata);
        ((myTestWin *)userdata)->cb_timer();
    }

    ~myTestWin() {
        delete this->window;  // deletes children recursively
    }
    aCCb::plot2d *tb;

   protected:
    aCCbWidget *menu;
    Fl_Double_Window *window;
    syncFile_t syncfile;
    string persistfile;
    string persistFileContents;
};

//* returns file contents split by whitespace */
vector<string> readPersistFileTokens(const string &filename) {
    std::ifstream is(filename, std::ios::binary);
    if (!is)
        return vector<string>();  // persistfile does not exist the first time, will be created

    // === file to string ===
    std::ostringstream all;
    all << is.rdbuf();
    string content = all.str();

    // === split at whitespace ===
    std::regex r("\\s+");  // cannot use a temporary expression (one-liner)
    return {std::sregex_token_iterator(content.begin(), content.end(), r, -1), /*equiv. to end()*/ std::sregex_token_iterator()};
}

void usage() {
    cerr << "usage:" << endl;
    cerr << "-trace" << endl;
    cerr << "   -dataX (filename)" << endl;
    cerr << "   -dataY (filename)" << endl;
    cerr << "   -vertLineY (number) repeated use is allowed" << endl;
    cerr << "   -horLineX (number) repeated use is allowed" << endl;
    cerr << "   -marker (e.g. w.1 see [1])" << endl;
    cerr << "   -annot (filenameTxt)" << endl;
    cerr << "   -annot2 (filenameIndex) (filenameTxt)" << endl;
    cerr << "   -mask (filename) (value)" << endl;
    cerr << "-xlabel (text)" << endl;
    cerr << "-ylabel (text)" << endl;
    cerr << "-title (text)" << endl;
    cerr << "-sync (filename)" << endl;
    cerr << "-persist (filename)" << endl;
    cerr << endl;
    cerr << "[1] colors in place of 'w': krgbcmyaow" << endl;
    cerr << "    shapes in place of '.1': .1 .2 .3 +1 +2 x1 x2 ('1' can be omitted')" << endl;
}

#if 0
class ww : public Fl_Double_Window {
   public:
    ww(int w, int h)  : Fl_Double_Window(w, h) {
    }
    void resize(int x, int y, int w, int h) {
        Fl_Double_Window::resize(x, y, w, h);
    }
};
#endif

static myTestWin *windowForSigIntHandler;
int main2(int argc, const char **argv) {
#if 0
    ww *window = new ww(111, 112);
    window->size(113, 114);
    cout << window->w() << "\t" << window->h() << "\t" << window->decorated_w() << "\t" << window->decorated_h() << endl;
    window->show();
    cout << window->w() << "\t" << window->h() << "\t" << window->decorated_w() << "\t" << window->decorated_h() << endl;
    window->size(113, 114);
    cout << window->w() << "\t" << window->h() << "\t" << window->decorated_w() << "\t" << window->decorated_h() << endl;
    Fl::run();
    cout << window->w() << "\t" << window->h() << "\t" << window->decorated_w() << "\t" << window->decorated_h() << endl;
    window->hide();
#endif
#if 0
#if 0
    const char *tmp[] = {"execname",
                         "-trace", "-dataY", "out2.float", "-marker", "g.3",
                         "-trace", "-dataY", "y.txt", "-dataX", "x.txt", "-marker", "wx1", /*"-vertLineY", "-1", "-vertLineY", "1",*/ "-annot", "x.txt",
                         "-trace", "-vertLineX", "-3", "-vertLineX", "3", "-horLineY", "-3", "-horLineY", "3", "-marker", "o.1",
                         "-title", "this is the title!", "-xlabel", "the xlabel", "-ylabel", "and the ylabel", "-xLimHigh", "-200000", "-sync", "b.txt",
                         // "-windowX", "10", "-windowY", "20", "-windowW", "1800", "-windowH", "1000",
                         "-persist", "c.txt"};
#elif 0
    const char *tmp[] = {"execname",
                         "-trace", "-marker", "g.3", "-dataX", "x.txt", "-dataY", "y.txt", "-sync", "b.txt", "-persist", "c.txt"};
#else
    const char *tmp[] = {"execname",
                         "-testcase", "5"};
#endif
    if (argc < 2) {
        cout << "*** debug cmd line args ***" << endl;
        argv = tmp;
        argc = sizeof(tmp) / sizeof(tmp[0]);
        for (int ix = 1; ix < argc; ++ix)
            cout << argv[ix] << " ";
        cout << endl;
    }
#else
    if (argc < 2) {
        usage();
        exit(0);
    }
#endif

    Fl::visual(FL_RGB);

    //* collects command line arguments */
    fooplotCmdLineArgRoot l;

    // === parse command line args ===
    for (int ixArg = 1; ixArg < argc; ++ixArg) {
        const string a = argv[ixArg];
        // cout << "parsing " << a << endl;
        if (!l.acceptArg(a))
            throw aCCb::argObjException("unexpected argument '" + a + "'");
    }

    if (l.showUsage) {
        usage();  // note: -usage, even at the end, is handled before any args processing that could throw an exception
        exit(/*EXIT_SUCCESS*/ 0);
    }

    // === read and apply "persistent" settings e.g. window position ===
    // those are applied after all command line args have been handled
    // (use model: delete persist file to reset)
    if (l.persistfile != "") {
        vector<string> tokens = readPersistFileTokens(l.persistfile);
        for (string v : tokens)
            if (!l.acceptArg(v))
                throw aCCb::argObjException("persist file error (" + l.persistfile + ") : unexpected token '" + v + "'");
    }

    l.close();

    // testcase replaces whole arguments set
    if (l.testcase >= 0)
        l = testcase(l.testcase);

    // === variables below must remain in scope until shutdown ===

    //* stores all trace data */
    traceDataMan_cl traceDataMan;

    //* provides all markers */
    markerMan_cl markerMan;

    //* all traces */
    allDrawJobs_cl allDrawJobs;

    for (auto t : l.traces) {
        const marker_cl *marker = markerMan.getMarker(t.marker);
        if (!marker)
            throw aCCb::argObjException("invalid marker description '" + t.marker + "'. Valid example: g.1");

        vector<drawJob::annotation_cl> annotations;
        for (annot2args &aInput : t.annotations) {
            const vector<uint32_t> *pMapping = NULL;
            if (aInput.mapFilename != "")
                pMapping = traceDataMan.getUInt32Vec(aInput.mapFilename);
            drawJob::annotation_cl aData(pMapping, traceDataMan.getAsciiVec(aInput.annotTxtFilename));
            annotations.push_back(aData);
        }

        //* one trace */
        drawJob j(
            traceDataMan.getFloatVec(t.dataX),
            traceDataMan.getFloatVec(t.dataY),
            annotations,
            marker,
            t.vertLineX,
            t.horLineY,
            traceDataMan.getUInt16Vec(t.maskFile),
            t.maskVal);

        allDrawJobs.addDrawJob(j);
    }

    // === start up window ===
    // background thread running
    myTestWin w(l, allDrawJobs);
    windowForSigIntHandler = &w;
    w.show();

    // === main loop ===
    Fl::run();

    // === shutdown ===
    // background thread reaped
    w.shutdown();
    return 0;
}

// Ctrl-C callback
void sigIntHandler(int /*signal*/) {
    // TODO 
    // "The safe and portable approach is never to call show() or hide() on any widget from the context of a worker thread. Instead you can use the Fl_Awake_Handler variant of Fl::awake() to request the main() thread to create, destroy, show or hide the widget on behalf of the worker thread"
    std::cout << "sig INT detected, shutting down" << endl;
    if (windowForSigIntHandler)
        windowForSigIntHandler->cb_close();  // same as regular close
    exit(1);
}

int main(int argc, const char **argv) {
    windowForSigIntHandler = NULL;
    signal(SIGINT, sigIntHandler);

    try {
        main2(argc, argv);
    } catch (aCCb::argObjException &e) {
        cerr << "error: " << e.what() << "\n";
        cerr << "use -help for usage information" << endl;
        exit(/*EXIT_FAILURE*/ -1);
    } catch (std::exception &e) {
        cerr << "unhandled exception: " << e.what() << endl;
        exit(/*EXIT_FAILURE*/ -1);
    }
    return 0;
}