
#pragma once

#include <string>
#include "LogicalObject.h"

struct GifInputData;

class GifInputObject : public LogicalObject {
    friend struct GifInputData;

public:
    GifInputObject(const std::string &name, const std::string &filename = std::string());
    GifInputObject(const GifInputObject &) = delete;
    virtual ~GifInputObject();
    GifInputObject & operator=(const GifInputObject &) = delete;
    GifInputObject * reconfigure(const std::string &filename = std::string());
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat) override;
    virtual bool getSize(int &width, int &height) const override;
    virtual bool acceptsFiles() const override;
    virtual bool loadFile(const char *filename) override;
    virtual void unloadFile() override;
    virtual bool restart() override;
    virtual bool pixelsReady() const override;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height) override;

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
