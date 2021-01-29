
#include "GifInputObject.h"

#include <gif_lib.h>
#include "open-utf8.h"

struct GifInputData {
    GifFileType *gif;
    uint32_t *frame;
    int fw, fh;
    int curFrame;
    int curFrameDuration;
    int curFrameEndTime;
};

GifInputObject::GifInputObject(const std::string &name, const std::string &filename) : LogicalObject(name), data(new GifInputData), initialFilename(filename) {
    prepared = false;
    repeat = false;
    data->gif = NULL;
    data->frame = NULL;
    data->fw = 0;
    data->fh = 0;
    data->curFrame = -1;
    data->curFrameDuration = 0;
    data->curFrameEndTime = 0;
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
                data->curFrame = -1;
                data->curFrameDuration = 0;
                data->curFrameEndTime = 0;
                frameRemainingTime = 0;
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
    data->fw = 0;
    data->fh = 0;
    data->curFrame = -1;
    data->curFrameDuration = 0;
    data->curFrameEndTime = 0;
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
    gcb.DisposalMode = DISPOSE_BACKGROUND;
    gcb.UserInputFlag = 0;
    gcb.DelayTime = 4;
    gcb.TransparentColor = -1;
    DGifSavedExtensionToGCB(data->gif, frameNo, &gcb);
    data->curFrameDuration = gcb.DelayTime;
    data->curFrameEndTime += gcb.DelayTime;
    bool keepFrame = gcb.DisposalMode == DISPOSE_DO_NOT && frameNo > 0;
    const ColorMapObject *colorMap = data->gif->SavedImages[frameNo].ImageDesc.ColorMap;
    if (!colorMap)
        colorMap = data->gif->SColorMap;
    const GifImageDesc &imageDesc = data->gif->SavedImages[frameNo].ImageDesc;
    const GifByteType *src = data->gif->SavedImages[frameNo].RasterBits;
    if (!colorMap || !src)
        return false;
    if (!keepFrame && (imageDesc.Left > 0 || imageDesc.Width < data->fw || imageDesc.Top > 0 || imageDesc.Height < data->fh))
        memset(data->frame, 0, sizeof(uint32_t)*data->fw*data->fh);
    for (int y = imageDesc.Top; y < data->fh && y < imageDesc.Top+imageDesc.Height; ++y) {
        uint32_t *dst = data->frame+data->fw*(data->fh-y-1)+imageDesc.Left;
        for (int x = imageDesc.Left; x < data->fw && x < imageDesc.Left+imageDesc.Width; ++x) {
            int index = int(*src++);
            GifColorType c = colorMap->Colors[index];
            if (index == gcb.TransparentColor) {
                if (!keepFrame)
                    *dst = uint32_t(c.Red)|uint32_t(c.Green)<<8|uint32_t(c.Blue)<<16;
            } else
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
