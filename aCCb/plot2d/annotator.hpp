#pragma once
//#include <unistd.h>  // usleep

#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>

#include "drawJob.hpp"
class annotator_t {
   public:
    annotator_t(allDrawJobs_cl& dj, void (*cb)(void* data), void* data) : dj(dj), callbackFun(cb), userdata(data) {
        bgTask = std::async(backgroundProcessWrapper, this);
    }

    void notifyCursorChange(double dataX, double dataY, proj<float>& p) {
        this->p = p;  // make a copy
        std::unique_lock<std::mutex> lock(mtx);
        mtState.cursorDataX = dataX;
        mtState.cursorDataY = dataY;
        ++mtState.trigger;
        cv.notify_one();
    }

    bool getHighlightedPoint(size_t& ixTrace, size_t& ixPt) {
        std::unique_lock<std::mutex> lock(mtx);
        // if (mtState.trigger != mtState.lastTrigger)
        //     return false;  // not started or running
        if (!mtState.resultIsValid)
            return false;  // finished but did not return a valid result
        ixTrace = mtState.ixTrace;
        ixPt = mtState.ixPt;
        return true;  // result is valid
    }

    // annotator may not be running before any other destructors are called, as it accesses
    // data of allDrawJobs (otherwise segfault). shutdown() waits until the background process
    // has been stopped.
    void shutdown() {
        if (isShutdown)
            throw std::runtime_error("annotator is already shutdown");
        isShutdown = true;

        {
            std::unique_lock<std::mutex> lock(mtx);
            mtState.keepRunning = 0;
        }
        cv.notify_one();
        bgTask.get();
    }

   protected:
    struct mtState_t {
        int trigger = 0;
        int lastTrigger = 0;
        float cursorDataX = std::numeric_limits<float>::infinity();
        float cursorDataY = std::numeric_limits<float>::infinity();
        int keepRunning = 1;
        bool resultIsValid = false;
        size_t ixTrace = std::numeric_limits<size_t>::max();
        size_t ixPt = std::numeric_limits<size_t>::max();
    } mtState;

    void backgroundProcess() {
        while (true) {
            while (true) {
                mtState_t stateCopy;
                {  // lock
                    std::unique_lock<std::mutex> lock(mtx);
                    stateCopy = mtState;
                }  // lock
                if (!stateCopy.keepRunning) {
                    return;
                }
                if (stateCopy.trigger == stateCopy.lastTrigger) {
                    break;
                }

                // Debug: simulate long lookup. 
                // Note: The perceived delay will be twice this amount, as the first trigger happens when the mouse moves a single pixel, 
                // usually returning the original point and showing no change.
                // usleep(1e6); // needs <unistd.h>
                stateCopy.resultIsValid = dj.findClosestPoint((float)stateCopy.cursorDataX, (float)stateCopy.cursorDataY, p, /*out*/ stateCopy.ixTrace, /*out*/ stateCopy.ixPt);

                {                                            // lock
                    std::unique_lock<std::mutex> lock(mtx);  // lock to closing bracket
                    mtState.resultIsValid = stateCopy.resultIsValid;
                    mtState.ixTrace = stateCopy.ixTrace;
                    mtState.ixPt = stateCopy.ixPt;
                    mtState.lastTrigger = stateCopy.trigger;  // this is the trigger state that initiated processing

                    // invoke callback
                    if (callbackFun != NULL)
                        callbackFun(userdata);
                }  // lock
            }      // while working

            // === wait for notification ===
            {
                std::unique_lock<std::mutex> lock(mtx);  // lock to closing bracket
                cv.wait(lock);
            }
        }  // while keepRunning
    }

    // starts background process (wrapper object method call via std::async)
    static void
    backgroundProcessWrapper(annotator_t* this_) {
        this_->backgroundProcess();
    }

    allDrawJobs_cl& dj;
    proj<float> p;
    std::mutex mtx;
    std::condition_variable cv;
    std::future<void> bgTask;
    bool isShutdown = false;
    // function to call when point under cursor has been identified
    void (*callbackFun)(void* userdata);
    // payload for callbackFun
    void* userdata;
};