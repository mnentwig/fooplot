#pragma once
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace aCCb {
using std::string;
// ==============================================================================
// command line parser: base class for argument
// ==============================================================================

// exception during command line parsing. Expected for incorrect user input.
class argObjException : public std::exception {
   public:
    argObjException(const string &msg) : msg(msg) {}
    const char *what() const noexcept {
        return msg.c_str();
    }

    // protected:
    const string msg;
};

// lightweight class for parsing and passing of command line arguments
// implementations derive from argObj (both root and nested levels)
// see STDFooPlot for example
class argObj {
   public:
    argObj(string token) : token(token), state(""), stack(), closed(false) {}

    // shorthand for throwing an exception during command line parsing on incorrect input
    // e.g. msg="failed to parse number: xyz"
    argObjException aoException(const string &msg) {
        throw argObjException("(" + token + "): " + msg);
    }

    // needs to be implemented at custom class level
    // default error handler by passing it back to base class (if this gets called, it's a bug)
    virtual bool acceptArg_stateSet(const string & /*arg*/) {
        throw std::runtime_error("?? " + token + ":state implementation is missing for '" + state + "' state ??");
    }

    // custom class calls this for an argument before trying to use it
    // (children are newer on the command line, therefore they get offered the input first)
    virtual bool acceptArg_stateUnset(const string &arg) {
        while (stack.size() > 0) {
            argObj *child = stack.back();
            // === feed the argument to the topmost element on the stack ===
            if (child->acceptArg(arg))
                return true;  // child accepted (and stays open)

            // === child is implicitly closed ===
            child->close();
            stack.pop_back();
        }

        // === handle common close argument '-end' ===
        // mostly debug feature when working with the application to avoid invisible implicit close of a level
        if (arg == "-end") {
            assert(stack.size() == 0);  // any child has been offered the arg, rejected it and was therefore closed
            closed = true;              // note, our removal from the parent's stack is delayed
            return true;
        }

        // neither this object nor younger objects stacked onto it were able to parse this argument
        return false;
    }
    void close() {
        if (!closed) {
            if (state != "") throw aoException(state + ": expecting argument");
            closed = true;
            while (stack.size() > 0) {
                stack.back()->close();
                stack.pop_back();
            }
        }
    }

    bool acceptArg(const string &arg) {
        if (closed)
            return false;
        if (state == "")
            return acceptArg_stateUnset(arg);
        else
            return acceptArg_stateSet(arg);
    }

   protected:
    // friendly name for messages only
    string token;
    // If the next argument is expected, 'state' denotes its purpose
    // e.g. "-myparam 100" results first in state=="-myparam", then back to default ""
    string state;
    // if the youngest object does not understand an argument it will pass it back to older objects
    // defined earlier on the command line. Any such object not accepting the passed-down argument is closed.
    // The arg is passed back to root level. If still not understood => argObjException
    std::vector<argObj *> stack;
    bool closed;
};
}  // namespace aCCb