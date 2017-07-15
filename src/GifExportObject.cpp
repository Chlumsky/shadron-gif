
#include "GifExportObject.h"

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <gif_lib.h>
#include "quantize.h"

struct GifExportData {
    GifFileType *gif;
    unsigned char *indexedBitmap;
    int w, h;
    uint32_t palette[256];
    int paletteSize;
    int frameDelay;
};

GifExportObject::GifExportObject(int sourceId, const std::string &filename, float framerate, float duration, bool repeat) : LogicalObject(std::string()), data(new GifExportData), sourceId(sourceId), filename(filename), framerate(framerate), duration(duration), repeat(repeat) {
    frameCount = (int) ceilf(framerate*duration);
    frameDuration = 1.f/framerate;
    step = -1;
    data->gif = NULL;
    data->indexedBitmap = NULL;
    data->w = 0, data->h = 0;
    memset(data->palette, 0, sizeof(data->palette));
    data->paletteSize = 0;
    data->frameDelay = (int) roundf(100.f/framerate);
}

GifExportObject::~GifExportObject() {
    finishExport();
    free(data->indexedBitmap);
    delete data;
}

int GifExportObject::offerSource(void *&pixelBuffer, int sourceId, int width, int height) {
    return sourceId == this->sourceId;
}

void GifExportObject::setSourcePixels(int sourceId, const void *pixels, int width, int height) {
    if (sourceId == this->sourceId && step >= 0) {
        if (step == 0) {
            data->indexedBitmap = reinterpret_cast<unsigned char *>(realloc(data->indexedBitmap, width*height));
            data->w = width, data->h = height;
        }
        if (data->indexedBitmap && width == data->w && height == data->h) {
            int colorCount = 256;
            quantize(data->indexedBitmap, data->palette, pixels, width, height, colorCount, true);
            int ceiledCount = 256;
            while ((ceiledCount>>1) >= colorCount && ceiledCount > 2)
                ceiledCount >>= 1;
            memset(data->palette+colorCount, 0, (ceiledCount-colorCount)*sizeof(*data->palette));
            data->paletteSize = ceiledCount;
        }
    }
}

bool GifExportObject::startExport() {
    step = -1;
    return true;
}

void GifExportObject::finishExport() {
    if (data->gif) {
        EGifCloseFile(data->gif, NULL);
        data->gif = NULL;
    }
}

int GifExportObject::getExportStepCount() const {
    return frameCount;
}

std::string GifExportObject::getExportFilename() const {
    return filename;
}

bool GifExportObject::prepareExportStep(int step, float &time, float &deltaTime) {
    time = step/framerate;
    deltaTime = frameDuration;
    this->step = step;
    return true;
}

bool GifExportObject::exportStep(int step) {
    if (data->indexedBitmap) {
        GraphicsControlBlock gcb;
        gcb.DisposalMode = DISPOSE_BACKGROUND;
        gcb.UserInputFlag = false;
        gcb.DelayTime = data->frameDelay;
        gcb.TransparentColor = NO_TRANSPARENT_COLOR;
        GifColorType colors[256];
        for (int i = 0; i < data->paletteSize; ++i) {
            colors[i].Red = GifByteType(data->palette[i]&0xff);
            colors[i].Green = GifByteType(data->palette[i]>>8&0xff);
            colors[i].Blue = GifByteType(data->palette[i]>>16&0xff);
            if (!(data->palette[i]&0xff000000))
                gcb.TransparentColor = i;
        }
        ColorMapObject *colorMap = GifMakeMapObject(data->paletteSize, colors);
        if (step == 0) {
            if ((data->gif = EGifOpenFileName(filename.c_str(), false, NULL))) {
                data->gif->SWidth = data->w;
                data->gif->SHeight = data->h;
                data->gif->SColorResolution = 8;
                data->gif->SBackGroundColor = -1;
                data->gif->AspectByte = 0;
                data->gif->SColorMap = colorMap;
                colorMap = NULL;
                unsigned char repeatExtensions[] = { 0x4e, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45, 0x32, 0x2e, 0x30, 0x01, !repeat, 0x00 };
                GifAddExtensionBlock(&data->gif->ExtensionBlockCount, &data->gif->ExtensionBlocks, 0xff, 11, repeatExtensions);
                GifAddExtensionBlock(&data->gif->ExtensionBlockCount, &data->gif->ExtensionBlocks, 0x00, 3, repeatExtensions+11);
            }
        }
        if (data->gif) {
            SavedImage *image = GifMakeSavedImage(data->gif, NULL);
            if (image) {
                image->ImageDesc.Left = 0;
                image->ImageDesc.Top = 0;
                image->ImageDesc.Width = data->w;
                image->ImageDesc.Height = data->h;
                image->ImageDesc.Interlace = 0;
                image->ImageDesc.ColorMap = colorMap;
                image->RasterBits = reinterpret_cast<GifByteType *>(malloc(data->w*data->h));
                for (int y = 0; y < data->h; ++y)
                    memcpy(image->RasterBits+y*data->w, data->indexedBitmap+(data->h-y-1)*data->w, data->w);
                EGifGCBToSavedExtension(&gcb, data->gif, step);
                if (step == frameCount-1)
                    if (EGifSpew(data->gif) == GIF_OK)
                        data->gif = NULL;
                return true;
            }
        }
        GifFreeMapObject(colorMap);
    }
    return false;
}
