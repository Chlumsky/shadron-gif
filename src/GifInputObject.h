
#pragma once

#include <string>
#include "LogicalObject.h"

class GifInputObject : public LogicalObject {
    friend struct GifInputData;

public:
    GifInputObject(const std::string &name, const std::string &filename = std::string());
    GifInputObject(const GifInputObject &) = delete;
    virtual ~GifInputObject();
    GifInputObject & operator=(const GifInputObject &) = delete;
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat);
    virtual bool getSize(int &width, int &height) const;
    virtual bool acceptsFiles() const;
    virtual bool loadFile(const char *filename);
    virtual void unloadFile();
    virtual bool restart();
    virtual bool pixelsReady() const;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height);

private:
    GifInputData *data;
    bool prepared;
    bool repeat;
    std::string initialFilename;

    double frameRemainingTime;

    void rewind();
    bool nextFrame();
    bool isFrameCurrent(float time, bool realTime);

};
