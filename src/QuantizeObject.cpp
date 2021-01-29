
#include "QuantizeObject.h"

#include <cstdlib>
#include <cstdint>
#include "quantize.h"

QuantizeObject::QuantizeObject(const std::string &name, int sourceId, int exprCountId) : LogicalObject(name), sourceId(sourceId), exprCountId(exprCountId) {
    count = 256;
    w = 0;
    h = 0;
    srcBitmap = NULL;
    bitmap = NULL;
}

QuantizeObject::~QuantizeObject() {
    free(srcBitmap);
    free(bitmap);
}

QuantizeObject * QuantizeObject::reconfigure(int sourceId, int exprCountId) {
    this->sourceId = sourceId;
    this->exprCountId = exprCountId;
    return this;
}

bool QuantizeObject::prepare(int &width, int &height, bool hardReset, bool repeat) {
    return getSize(width, height);
}

bool QuantizeObject::getSize(int &width, int &height) const {
    width = w;
    height = h;
    return true;
}

bool QuantizeObject::acceptsFiles() const {
    return false;
}

int QuantizeObject::setExpressionValue(int exprId, int type, const void *value) {
    if (exprId == exprCountId && type == TYPE_INT) {
        int v = *reinterpret_cast<const int *>(value);
        if (v < 2) v = 2;
        if (v > 256) v = 256;
        if (v != count) {
            count = v;
            render();
            return true;
        }
    }
    return false;
}

int QuantizeObject::offerSource(void *&pixelBuffer, int sourceId, int width, int height) {
    if (sourceId == this->sourceId) {
        if (w != width || h != height) {
            w = width;
            h = height;
            srcBitmap = realloc(srcBitmap, 4*w*h);
            bitmap = realloc(bitmap, 4*w*h);
            pixelBuffer = srcBitmap;
            return OBJ_RESIZE;
        }
        pixelBuffer = srcBitmap;
        return true;
    }
    return false;
}

void QuantizeObject::setSourcePixels(int sourceId, const void *pixels, int width, int height) {
    if (pixels == srcBitmap && width == w && height == h)
        render();
}

bool QuantizeObject::pixelsReady() const {
    return bitmap != NULL;
}

const void * QuantizeObject::fetchPixels(float time, float deltaTime, bool realTime, int width, int height) {
    if (width != w || height != h)
        return NULL;
    return bitmap;
}

void QuantizeObject::render() {
    if (w > 0 && h > 0) {
        uint32_t palette[256] = { };
        unsigned char *indices = reinterpret_cast<unsigned char *>(malloc(w*h));
        int colorCount = count;
        quantize(indices, palette, srcBitmap, w, h, colorCount, false);
        const unsigned char *ind = indices;
        uint32_t *pxl = reinterpret_cast<uint32_t *>(bitmap);
        for (int i = 0; i < w*h; ++i)
            *pxl++ = palette[*ind++];
        free(indices);
    }
}
