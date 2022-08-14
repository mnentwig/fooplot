#pragma once
#include <filesystem>
#include <string>

using std::string, std::vector;

// loads data files, converts format and keeps them in memory, providing const pointers via filename.
class traceDataMan_cl {
    // converts vector of arbitrary type to float by casting
    template <typename T>
    static vector<float> floatify(const vector<T> &data) {
        vector<float> r(data.size());
        for (size_t ix = 0; ix < data.size(); ++ix)
            r[ix] = (float)data[ix];
        return r;
    }

    // converts vector by casting elements individually
    template <typename Tin, typename Tout>
    static vector<Tout> castVector(const vector<Tin> &data) {
        vector<Tout> r(data.size());
        for (size_t ix = 0; ix < data.size(); ++ix)
            r[ix] = (Tout)data[ix];
        return r;
    }

    // converts vector to uint16_t by casting elements individually
    template <typename Tin>
    static vector<uint16_t> castVectorToUint16(const vector<Tin> &data) {
        return castVector<Tin, uint16_t>(data);
    }

    // converts vector to uint32_t by casting elements individually
    template <typename Tin>
    static vector<uint32_t> castVectorToUint32(const vector<Tin> &data) {
        return castVector<Tin, uint32_t>(data);
    }

   public:
    traceDataMan_cl() {}

    // returns contents of a given datafile as 32-bit float vector
    const vector<float> *getFloatVec(const string &filename) {
        if (filename == "")
            return NULL;

        string fnCan = std::filesystem::canonical(filename).string();
        loadAsFloat(fnCan);  // note: underlying 'map' container does NOT invalidate iterators on insertion (= pointers previously returned)
        return &floatDataByFilename.at(fnCan);
    }

    // returns contents of a given datafile as 16-bit unsigned vector
    const vector<uint16_t> *getUInt16Vec(const string &filename) {
        if (filename == "")
            return NULL;

        string fnCan = std::filesystem::canonical(filename).string();
        loadAsUInt16(fnCan);  // note: underlying 'map' container does NOT invalidate iterators on insertion (= pointers previously returned)
        return &uint16DataByFilename.at(fnCan);
    }

    // returns contents of a given datafile as 32-bit unsigned vector
    const vector<uint32_t> *getUInt32Vec(const string &filename) {
        if (filename == "")
            return NULL;

        string fnCan = std::filesystem::canonical(filename).string();
        loadAsUInt32(fnCan);  // note: underlying 'map' container does NOT invalidate iterators on insertion (= pointers previously returned)
        return &uint32DataByFilename.at(fnCan);
    }

    // returns contents of a given file (e.g. annotations) as literal ASCII text
    const vector<string> *getAsciiVec(const string &filename) {
        if (filename == "")
            return NULL;

        string fnCan = std::filesystem::canonical(filename).string();
        loadAsLiteralAscii(fnCan);
        return &asciiDataByFilename.at(fnCan);
    }

#if 0
    const vector<const vector<string> *> getAsciiVecs(const vector<string> &filenames) {
        vector<const vector<string> *> r;
        for (const string &filename : filenames)
            r.push_back(getAsciiVec(filename));
        return r;
    }
#endif

   protected:
    map<string, vector<float>> floatDataByFilename;
    map<string, vector<uint16_t>> uint16DataByFilename;
    map<string, vector<uint32_t>> uint32DataByFilename;
    map<string, vector<string>> asciiDataByFilename;

    // loads data for retrieval by its filename as 32-bit float vector
    void loadAsFloat(const string &filename) {
        if (floatDataByFilename.count(filename) > 0)
            return;

        // file extension identifies input data format
        string ext = std::filesystem::path(filename).extension().string();

        if (aCCb::caseInsensitiveStringCompare(".float", ext))
            floatDataByFilename[filename] = file2vec<float>(filename);
        else if (aCCb::caseInsensitiveStringCompare(".double", ext))
            floatDataByFilename[filename] = floatify(file2vec<double>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int8", ext))
            floatDataByFilename[filename] = floatify(file2vec<int8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint8", ext))
            floatDataByFilename[filename] = floatify(file2vec<uint8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int16", ext))
            floatDataByFilename[filename] = floatify(file2vec<int16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint16", ext))
            floatDataByFilename[filename] = floatify(file2vec<uint16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int32", ext))
            floatDataByFilename[filename] = floatify(file2vec<int32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint32", ext))
            floatDataByFilename[filename] = floatify(file2vec<uint32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int64", ext))
            floatDataByFilename[filename] = floatify(file2vec<int64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint64", ext))
            floatDataByFilename[filename] = floatify(file2vec<uint64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".txt", ext))
            floatDataByFilename[filename] = loadFloatVecFromTxt(filename);
        else
            throw aCCb::argObjException("unsupported data file extension (" + filename + ")");
    }

    // loads data for retrieval by its filename as 16-bit unsigned integer vector
    void loadAsUInt16(const string &filename) {
        if (filename == "")
            return;
        if (uint16DataByFilename.count(filename) > 0)
            return;

        // file extension identifies input data format
        string ext = std::filesystem::path(filename).extension().string();

        if (aCCb::caseInsensitiveStringCompare(".float", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<float>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".double", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<double>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int8", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<int8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint8", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<uint8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int16", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<int16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint16", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<uint16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int32", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<int32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint32", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<uint32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int64", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<int64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint64", ext))
            uint16DataByFilename[filename] = castVectorToUint16(file2vec<uint64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".txt", ext))
            uint16DataByFilename[filename] = castVectorToUint16(loadFloatVecFromTxt(filename));
        else
            throw aCCb::argObjException("unsupported data file extension (" + filename + ")");
    }

    // loads data for retrieval by its filename as 32-bit unsigned integer vector
    void loadAsUInt32(const string &filename) {
        if (filename == "")
            return;
        if (uint32DataByFilename.count(filename) > 0)
            return;

        // file extension identifies input data format
        string ext = std::filesystem::path(filename).extension().string();

        if (aCCb::caseInsensitiveStringCompare(".float", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<float>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".double", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<double>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int8", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<int8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint8", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<uint8_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int16", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<int16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint16", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<uint16_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int32", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<int32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint32", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<uint32_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".int64", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<int64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".uint64", ext))
            uint32DataByFilename[filename] = castVectorToUint32(file2vec<uint64_t>(filename));
        else if (aCCb::caseInsensitiveStringCompare(".txt", ext))
            uint32DataByFilename[filename] = castVectorToUint32(loadFloatVecFromTxt(filename));
        else
            throw aCCb::argObjException("unsupported data file extension (" + filename + ")");
    }

    // loads literal ASCII data for retrieval by its filename
    void loadAsLiteralAscii(const string &filename) {
        if (filename == "")
            return;
        if (asciiDataByFilename.count(filename) > 0)
            return;

        std::ifstream is(filename);
        if (!is.is_open()) throw runtime_error("failed to open file (r): '" + filename + "')");

        vector<string> &r = asciiDataByFilename[filename];  // creates new, empty vector
        string line;
        while (std::getline(is, line))
            r.push_back(line);
    }

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
