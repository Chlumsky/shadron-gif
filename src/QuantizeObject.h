
#pragma once

#include "LogicalObject.h"

class QuantizeObject : public LogicalObject {

public:
    QuantizeObject(const std::string &name, int sourceId, int exprCountId);
    QuantizeObject(const QuantizeObject &) = delete;
    virtual ~QuantizeObject();
    QuantizeObject & operator=(const QuantizeObject &) = delete;
    virtual bool prepare(int &width, int &height, bool hardReset, bool repeat);
    virtual bool getSize(int &width, int &height) const;
    virtual bool acceptsFiles() const;
    virtual int setExpressionValue(int exprId, int type, const void *value);
    virtual int offerSource(void *&pixelBuffer, int sourceId, int width, int height);
    virtual void setSourcePixels(int sourceId, const void *pixels, int width, int height);
    virtual bool pixelsReady() const;
    virtual const void * fetchPixels(float time, float deltaTime, bool realTime, int width, int height);

private:
    int sourceId;
    int exprCountId;

    int count;
    int w, h;
    void *srcBitmap;
    void *bitmap;

    void render();

};
