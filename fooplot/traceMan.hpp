#pragma once
#include <filesystem>
#include <string>

using std::string, std::vector;
class traceDataMan_cl {
    // converts vector of arbitrary type to float by casting
    template <typename T>
    static vector<float> floatify(const vector<T> data) {
        vector<float> r(data.size());
        for (size_t ix = 0; ix < data.size(); ++ix)
            r[ix] = (float)data[ix];
        return r;
    }

   public:
    traceDataMan_cl() {}
    void loadData(const string &filename) {
        if (filename == "")
            return;
        if (dataByFilename.find(filename) != dataByFilename.end())
            return;
        string ext = std::filesystem::path(filename).extension().string();
        if (aCCb::caseInsensitiveStringCompare(".float", ext))
            dataByFilename[filename] = file2vec<float>(filename);
        else if (aCCb::caseInsensitiveStringCompare(".double", ext))
            dataByFilename[filename] = floatify(file2vec<double>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int8", ext))
            dataByFilename[filename] = floatify(file2vec<int8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint8", ext))
            dataByFilename[filename] = floatify(file2vec<uint8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int16", ext))
            dataByFilename[filename] = floatify(file2vec<int16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint16", ext))
            dataByFilename[filename] = floatify(file2vec<uint16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int32", ext))
            dataByFilename[filename] = floatify(file2vec<int32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint32", ext))
            dataByFilename[filename] = floatify(file2vec<uint32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int64", ext))
            dataByFilename[filename] = floatify(file2vec<int64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint64", ext))
            dataByFilename[filename] = floatify(file2vec<uint64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".txt", ext))
            dataByFilename[filename] = loadFloatVecFromTxt(filename);
        else
            throw aCCb::argObjException("unsupported data file extension (" + filename + ")");
    }

    void loadAnnotations(const string &filename) {
        if (filename == "")
            return;

        vector<string> r;
        std::ifstream is(filename);
        if (!is.is_open()) throw runtime_error("failed to open file (r): '" + filename + "')");
        string line;
        while (std::getline(is, line))
            r.push_back(line);
        annotationsByFilename[filename] = r;
    }

    const vector<string> *getAnnotations(const string &filename) const {
        if (filename == "")
            return NULL;
        auto it = annotationsByFilename.find(filename);
        if (it == annotationsByFilename.end())
            throw runtime_error("datafile should have been loaded but does not exist");
        return &(it->second);
    }

    const vector<float> *getData(const string &filename) {
        if (filename == "")
            return NULL;
        auto it = dataByFilename.find(filename);
        if (it == dataByFilename.end())
            throw runtime_error("datafile should have been loaded but does not exist");
        return &(it->second);
    }

   protected:
    map<string, vector<float>> dataByFilename;
    map<string, vector<string>> annotationsByFilename;

    //* Read binary data from file into vector */
    template <class T>
    static vector<T> file2vec(const string fname) {
        std::ifstream is(fname, std::ifstream::binary);
        if (!is)
            throw runtime_error("failed to open file: " + fname);

        is.seekg(0, std::ios_base::end);
        std::size_t nBytes = is.tellg();
        size_t elemSize = sizeof(T);
        size_t nElem = nBytes / elemSize;
        if (nElem * elemSize != nBytes)
            throw runtime_error("binary file contains partial element: " + fname);

        is.seekg(0, std::ios_base::beg);
        vector<T> retVal(nElem);
        is.read((char *)&retVal[0], nElem * sizeof(T));
        if (!is)
            throw runtime_error("read failed");
        return retVal;
    }

    // read ASCII data from file into float vector. Non-parseable lines are returned as NAN, trailing data after a parseable number is ignored
    static vector<float> loadFloatVecFromTxt(const string &filename) {
        vector<float> r;
        r.reserve(1000000);  // smaller data is unlikely / "don't-care" wrt performance

        std::ifstream is(filename, std::ifstream::binary);
        if (!is)
            throw runtime_error("failed to open file: " + filename);

        string line;
        while (std::getline(is, line)) {
            const char *str = line.c_str();
            char *endptr;
            float val = std::strtof(str, &endptr);
            if (/*conversion failed*/ (endptr == str) || /*overrange */ (val == HUGE_VALF))
                val = std::numeric_limits<float>::quiet_NaN();
            r.push_back(val);
        }
        return r;
    }
};
