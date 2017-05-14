
#pragma once

#include <string>
#include "LogicalObject.h"

class GifExportObject : public LogicalObject {
    friend struct GifExportData;

public:
    GifExportObject(int sourceId, const std::string &filename, float framerate, float duration, bool repeat);
    GifExportObject(const GifExportObject &) = delete;
    virtual ~GifExportObject();
    GifExportObject & operator=(const GifExportObject &) = delete;
    virtual int offerSource(void *&pixelBuffer, int sourceId, int width, int height);
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height);
    virtual bool startExport();
    virtual void finishExport();
    virtual int getExportStepCount() const;
    virtual std::string getExportFilename() const;
    virtual bool prepareExportStep(int step, float &time, float &deltaTime);
    virtual bool exportStep(int step);

private:
    GifExportData *data;
    int sourceId;
    std::string filename;
    float framerate;
    float duration;
    bool repeat;
    int frameCount;
    float frameDuration;
    int step;

};
