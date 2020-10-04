
#pragma once

#include <cstdlib>
#include <string>
#include <map>
#include "LogicalObject.h"

#define EXTENSION_NAME "gif"
#define EXTENSION_VERSION 120

#define INITIALIZER_GIF_FILE_ID 0
#define INITIALIZER_GIF_FILE_NAME "gif_file"

#define INITIALIZER_GIF_EXPORT_ID 1
#define INITIALIZER_GIF_EXPORT_NAME "gif"

#define INITIALIZER_QUANTIZE_ID 2
#define INITIALIZER_QUANTIZE_NAME "quantize"

#define ERROR_EXPORT_SOURCE_TYPE "Only animation and image objects may be exported as GIF files"
#define ERROR_FRAMERATE_POSITIVE "The animation frame rate must be a positive floating point value"
#define ERROR_DURATION_NONNEGATIVE "The animation duration must be a positive time in seconds"
#define ERROR_QUANTIZE_RANGE "The number of colors must be between 2 and 256"

class GifExtension {

public:
    GifExtension();
    GifExtension(const GifExtension &) = delete;
    ~GifExtension();
    GifExtension & operator=(const GifExtension &) = delete;
    void refObject(LogicalObject *object);
    void unrefObject(LogicalObject *object);
    LogicalObject * findObject(const std::string &name) const;

private:
    std::map<LogicalObject *, int> objectRefs;
    std::map<std::string, LogicalObject *> objectLookup;

};
