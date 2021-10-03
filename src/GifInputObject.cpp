
#include "GifInputObject.h"

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <gif_lib.h>
#include "open-utf8.h"

struct FrameBounds {
    int x0, y0, x1, y1;
};

struct GifInputData {
    GifFileType *gif;
    uint32_t *frame;
    uint32_t *prevFrame;
    int fw, fh;
    int curFrame;
    int curFrameDuration;
    int curFrameEndTime;
    int curFrameDisposalMode;
    FrameBounds curFrameBounds;
};

static void clearArea(uint32_t *frame, int fw, int fh, const FrameBounds &frameBounds) {
    for (int y = frameBounds.y0; y < frameBounds.y1; ++y)
        memset(frame+fw*(fh-y-1)+frameBounds.x0, 0, sizeof(uint32_t)*(frameBounds.x1-frameBounds.x0));
}

static void copyArea(uint32_t *dstFrame, const uint32_t *srcFrame, int fw, int fh, const FrameBounds &frameBounds) {
    for (int y = frameBounds.y0; y < frameBounds.y1; ++y)
        memcpy(dstFrame+fw*(fh-y-1)+frameBounds.x0, srcFrame+fw*(fh-y-1)+frameBounds.x0, sizeof(uint32_t)*(frameBounds.x1-frameBounds.x0));
}

GifInputObject::GifInputObject(const std::string &name, const std::string &filename) : LogicalObject(name), data(new GifInputData), initialFilename(filename) {
    prepared = false;
    repeat = false;
    data->gif = NULL;
    data->frame = NULL;
    data->prevFrame = NULL;
    data->fw = 0;
    data->fh = 0;
    data->curFrame = -1;
    data->curFrameDuration = 0;
    data->curFrameEndTime = 0;
    data->curFrameDisposalMode = DISPOSAL_UNSPECIFIED;
    data->curFrameBounds = FrameBounds();
    frameRemainingTime = 0;
}

GifInputObject::~GifInputObject() {
    unloadFile();
    delete data;
}

GifInputObject * GifInputObject::reconfigure(const std::string &filename) {
    initialFilename = filename;
    return this;
}

bool GifInputObject::prepare(int &width, int &height, bool hardReset, bool repeat) {
    this->repeat = repeat;
    if (!prepared || hardReset) {
        if (!initialFilename.empty())
            loadFile(initialFilename.c_str());
        else
            unloadFile();
    }
    return prepared = getSize(width, height);
}

bool GifInputObject::getSize(int &width, int &height) const {
    if (data->gif) {
        width = data->fw;
        height = data->fh;
    } else {
        width = 0;
        height = 0;
    }
    return true;
}

bool GifInputObject::acceptsFiles() const {
    return true;
}

bool GifInputObject::loadFile(const char *filename) {
    if (!filename) {
        if (initialFilename.empty())
            return false;
        filename = initialFilename.c_str();
    }
    int fd = openUtf8(filename, O_RDONLY);
    if (fd != -1) {
        GifFileType *gif = DGifOpenFileHandle(fd, NULL);
        if (gif) {
            if (DGifSlurp(gif) == GIF_OK) {
                unloadFile();
                data->gif = gif;
                data->fw = gif->SWidth;
                data->fh = gif->SHeight;
                data->frame = new uint32_t[data->fw*data->fh];
                return true;
            }
            DGifCloseFile(gif, NULL);
        }
    }
    return false;
}

void GifInputObject::unloadFile() {
    if (data->frame) {
        delete [] data->frame;
        data->frame = NULL;
    }
    if (data->prevFrame) {
        delete [] data->prevFrame;
        data->prevFrame = NULL;
    }
    data->fw = 0;
    data->fh = 0;
    data->curFrame = -1;
    data->curFrameDuration = 0;
    data->curFrameEndTime = 0;
    data->curFrameDisposalMode = DISPOSAL_UNSPECIFIED;
    data->curFrameBounds = FrameBounds();
    frameRemainingTime = 0;
    if (data->gif) {
        DGifCloseFile(data->gif, NULL);
        data->gif = NULL;
    }
}

bool GifInputObject::restart() {
    if (data->gif && data->curFrame >= 0) {
        rewind();
        return true;
    }
    return false;
}

bool GifInputObject::pixelsReady() const {
    return data->gif != NULL;
}

void GifInputObject::rewind() {
    data->curFrame = -1;
    data->curFrameDuration = 0;
    data->curFrameEndTime = 0;
    frameRemainingTime = 0;
}

bool GifInputObject::nextFrame() {
    int frameNo = data->curFrame = (data->curFrame+1)%data->gif->ImageCount;
    GraphicsControlBlock gcb;
    gcb.DisposalMode = DISPOSAL_UNSPECIFIED;
    gcb.UserInputFlag = 0;
    gcb.DelayTime = 4;
    gcb.TransparentColor = NO_TRANSPARENT_COLOR;
    DGifSavedExtensionToGCB(data->gif, frameNo, &gcb);
    data->curFrameDuration = gcb.DelayTime;
    data->curFrameEndTime += gcb.DelayTime;
    const ColorMapObject *colorMap = data->gif->SavedImages[frameNo].ImageDesc.ColorMap;
    if (!colorMap)
        colorMap = data->gif->SColorMap;
    const GifImageDesc &imageDesc = data->gif->SavedImages[frameNo].ImageDesc;
    const GifByteType *raster = data->gif->SavedImages[frameNo].RasterBits;
    if (!colorMap || (!raster && imageDesc.Width > 0 && imageDesc.Height > 0))
        return false;
    FrameBounds frameBounds = {
        std::max(imageDesc.Left, 0),
        std::max(imageDesc.Top, 0),
        std::min(imageDesc.Left+imageDesc.Width, data->fw),
        std::min(imageDesc.Top+imageDesc.Height, data->fh)
    };
    if (gcb.TransparentColor >= 0 || frameBounds.x0 > 0 || frameBounds.x1 < data->fw || frameBounds.y0 > 0 || frameBounds.y1 < data->fh || gcb.DisposalMode == DISPOSE_PREVIOUS) {
        if (frameNo == 0)
            memset(data->frame, 0, sizeof(uint32_t)*data->fw*data->fh);
        else switch (data->curFrameDisposalMode) {
            case DISPOSE_PREVIOUS:
                if (data->prevFrame) {
                    copyArea(data->frame, data->prevFrame, data->fw, data->fh, data->curFrameBounds);
                    break;
                }
            case DISPOSE_BACKGROUND:
                clearArea(data->frame, data->fw, data->fh, data->curFrameBounds);
                break;
        }
    }
    data->curFrameDisposalMode = gcb.DisposalMode;
    data->curFrameBounds = frameBounds;
    if (data->curFrameDisposalMode == DISPOSE_PREVIOUS) {
        if (frameNo > 0) {
            if (!data->prevFrame)
                data->prevFrame = new uint32_t[data->fw*data->fh];
            copyArea(data->prevFrame, data->frame, data->fw, data->fh, data->curFrameBounds);
        } else
            data->curFrameDisposalMode = DISPOSE_BACKGROUND;
    }
    for (int y = frameBounds.y0; y < frameBounds.y1; ++y) {
        uint32_t *dst = data->frame+data->fw*(data->fh-y-1)+frameBounds.x0;
        const GifByteType *src = raster+imageDesc.Width*(y-imageDesc.Top)+(frameBounds.x0-imageDesc.Left);
        for (int x = frameBounds.x0; x < frameBounds.x1; ++x) {
            int index = int(*src++);
            index *= (unsigned) index < (unsigned) colorMap->ColorCount;
            GifColorType c = colorMap->Colors[index];
            if (index != gcb.TransparentColor)
                *dst = 0xff000000|uint32_t(c.Red)|uint32_t(c.Green)<<8|uint32_t(c.Blue)<<16;
            ++dst;
        }
    }
    return true;
}

bool GifInputObject::isFrameCurrent(float time, bool realTime) {
    if (data->curFrame < 0)
        return false;
    if (realTime)
        return frameRemainingTime > 0.0;
    else
        return 0.01*data->curFrameEndTime > (double) time;
}

const void * GifInputObject::fetchPixels(float time, float deltaTime, bool realTime, int width, int height) {
    if (width != data->fw || height != data->fh || !data->gif)
        return NULL;

    if (data->gif && width == data->fw && height == data->fh) {
        if (time == 0.f)
            rewind();
        frameRemainingTime -= (double) deltaTime;
        if (isFrameCurrent(time, realTime))
            return NULL;
        do {
            if (!nextFrame())
                return NULL;
            frameRemainingTime += 0.01*data->curFrameDuration;
        } while (!isFrameCurrent(time, realTime));
        return data->frame;
    }
    return NULL;
}
