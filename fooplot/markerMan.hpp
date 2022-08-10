#pragma once
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "../aCCb/plot2d/marker.hpp"
using std::string, std::vector, std::pair, std::map;
class markerMan_cl {
   public:
    markerMan_cl() {
        // =======================================
        // set up markers
        // =======================================
        const vector<pair<char, uint32_t>> colors{
            {'k', 0xFF222222},
            {'r', 0xFF0000FF},
            {'g', 0xFF00FF00},
            {'b', 0xFFFF0000},
            {'c', 0xFFFFFF00},
            {'m', 0xFFFF00FF},
            {'y', 0xFF00FFFF},
            {'a', 0xFF888888},
            {'o', 0xFF008cFF},  // orange
            {'w', 0xFFFFFFFF}};

        const string dot = ".";
        const string plus = "+";
        const string cross = "x";
        for (auto x : colors) {
            char colCode = x.first;
            uint32_t rgba = x.second;
            string sequence;
            string key;

            sequence = "X";
            key = colCode + dot + "1";
            markers[key] = new marker_cl(sequence, rgba);
            markers[colCode + dot] = markers[key];

            sequence =
                "XXX"
                "XXX"
                "XXX";
            key = colCode + dot + "2";
            markers[key] = new marker_cl(sequence, rgba);

            sequence =
                " XXX "
                "XXXXX"
                "XXXXX"
                "XXXXX"
                " XXX ";
            key = colCode + dot + "3";
            markers[key] = new marker_cl(sequence, rgba);

            sequence =
                " X "
                "XXX"
                " X ";
            key = colCode + plus + "1";
            markers[key] = new marker_cl(sequence, rgba);
            markers[colCode + plus] = markers[key];

            sequence =
                "  X  "
                "  X  "
                "XXXXX"
                "  X  "
                "  X  ";
            key = colCode + plus + "2";
            markers[key] = new marker_cl(sequence, rgba);

            sequence =
                "X X"
                " X "
                "X X";
            key = colCode + cross + "1";
            markers[key] = new marker_cl(sequence, rgba);
            markers[colCode + cross] = markers[key];

            sequence =
                "X   X"
                " X X "
                "  X  "
                " X X "
                "X   X";
            key = colCode + cross + "2";
            markers[key] = new marker_cl(sequence, rgba);

        }  // for colors
    }

    const marker_cl *getMarker(const string &desc) const {
        auto it = markers.find(desc);
        if (it == markers.end())
            return NULL;
        return it->second;
    }

    ~markerMan_cl() {
        // markers contains aliases e.g. "w." for "w.1"
        // collect unique markers
        std::set<marker_cl *> uniqueMarkers;
        for (auto v : markers)
            uniqueMarkers.insert(v.second);
        // ... and clean up
        for (auto v : uniqueMarkers)
            delete v;
    }

   protected:
    map<string, marker_cl *> markers;
};
