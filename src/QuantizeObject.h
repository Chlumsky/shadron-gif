
#pragma once

#include "LogicalObject.h"

class QuantizeObject : public LogicalObject {

public:
    QuantizeObject(const std::string &name, int sourceId, int exprCountId);
    QuantizeObject(const QuantizeObject &) = delete;
    virtual ~QuantizeObject();
    QuantizeObject & operator=(const QuantizeObject &) = delete;
    QuantizeObject * reconfigure(int sourceId, int exprCountId);
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat) override;
    virtual bool getSize(int &width, int &height) const override;
    virtual bool acceptsFiles() const override;
    virtual int setExpressionValue(int exprId, int type, const void *value) override;
    virtual int offerSource(void *&pixelBuffer, int sourceId, int width, int height) override;
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height) override;
    virtual bool pixelsReady() const override;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height) override;

private:
    int sourceId;
    int exprCountId;

    int count;
    int w, h;
    void *srcBitmap;
    void *bitmap;

    void render();

};
