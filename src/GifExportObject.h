
#pragma once

#include <string>
#include "LogicalObject.h"

class GifExportObject : public LogicalObject {

public:
    GifExportObject(int sourceId, const std::string &filename, float framerate, float duration, bool repeat);
    GifExportObject(const GifExportObject &) = delete;
    virtual ~GifExportObject();
    GifExportObject & operator=(const GifExportObject &) = delete;
    GifExportObject * reconfigure(int sourceId, const std::string &filename, float framerate, float duration, bool repeat);
    virtual int offerSource(void *&pixelBuffer, int sourceId, int width, int height) override;
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height) override;
    virtual bool startExport() override;
    virtual void finishExport() override;
    virtual int getExportStepCount() const override;
    virtual std::string getExportFilename() const override;
    virtual bool prepareExportStep(int step, float &time, float &deltaTime) override;
    virtual bool exportStep(int step) override;

private:
    struct GifExportData;

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
