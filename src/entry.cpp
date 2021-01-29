
#include <cstdlib>
#include <cstring>
#include <string>
#include <shadron-api.h>
#include "GifExtension.h"
#include "LogicalObject.h"
#include "GifInputObject.h"
#include "GifExportObject.h"
#include "QuantizeObject.h"

int SHADRON_VERSION;

struct ParseData {
    int initializer;
    int curArg;
    std::string filename;
    int sourceId;
    bool animated;
    int exprFramerate;
    int exprDuration;
    int exprRepeat;
    int exprColorCount;
    float framerate;
    float duration;
    bool repeat;
};

template <class T, typename... A>
static void reconfigure(LogicalObject *&obj, A... args) {
    T *subObj = dynamic_cast<T *>(obj);
    obj = subObj ? subObj->reconfigure(args...) : NULL;
}

extern "C" {

int SHADRON_API_FN shadron_register_extension(int *magicNumber, int *flags, char *name, int *nameLength, int *version, void **context) {
    SHADRON_VERSION = *version;
    *magicNumber = SHADRON_MAGICNO;
    *flags = SHADRON_FLAG_IMAGE|SHADRON_FLAG_ANIMATION|SHADRON_FLAG_EXPORT|SHADRON_FLAG_CHARSET_UTF8;
    if (*nameLength <= sizeof(EXTENSION_NAME))
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    memcpy(name, EXTENSION_NAME, sizeof(EXTENSION_NAME));
    *nameLength = sizeof(EXTENSION_NAME)-1;
    *version = EXTENSION_VERSION;
    *context = new GifExtension;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_unregister_extension(void *context) {
    GifExtension *ext = reinterpret_cast<GifExtension *>(context);
    delete ext;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_register_initializer(void *context, int index, int *flags, char *name, int *nameLength) {
    switch (index) {
        case INITIALIZER_GIF_FILE_ID:
            if (*nameLength <= sizeof(INITIALIZER_GIF_FILE_NAME))
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            memcpy(name, INITIALIZER_GIF_FILE_NAME, sizeof(INITIALIZER_GIF_FILE_NAME));
            *nameLength = sizeof(INITIALIZER_GIF_FILE_NAME)-1;
            *flags = SHADRON_FLAG_ANIMATION;
            return SHADRON_RESULT_OK;
        case INITIALIZER_GIF_EXPORT_ID:
            if (*nameLength <= sizeof(INITIALIZER_GIF_EXPORT_NAME))
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            memcpy(name, INITIALIZER_GIF_EXPORT_NAME, sizeof(INITIALIZER_GIF_EXPORT_NAME));
            *nameLength = sizeof(INITIALIZER_GIF_EXPORT_NAME)-1;
            *flags = SHADRON_FLAG_EXPORT;
            return SHADRON_RESULT_OK;
        case INITIALIZER_QUANTIZE_ID:
            if (*nameLength <= sizeof(INITIALIZER_QUANTIZE_NAME))
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            memcpy(name, INITIALIZER_QUANTIZE_NAME, sizeof(INITIALIZER_QUANTIZE_NAME));
            *nameLength = sizeof(INITIALIZER_QUANTIZE_NAME)-1;
            *flags = SHADRON_FLAG_IMAGE|SHADRON_FLAG_ANIMATION;
            return SHADRON_RESULT_OK;
        default:
            return SHADRON_RESULT_NO_MORE_ITEMS;
    }
}

int SHADRON_API_FN shadron_parse_initializer(void *context, int objectType, int index, const char *name, int nameLength, void **parseContext, int *firstArgumentTypes) {
    switch (index) {
        case INITIALIZER_GIF_FILE_ID:
            if (objectType != SHADRON_FLAG_ANIMATION)
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            *parseContext = new ParseData { INITIALIZER_GIF_FILE_ID };
            *firstArgumentTypes = SHADRON_ARG_NONE|SHADRON_ARG_FILENAME;
            return SHADRON_RESULT_OK;
        case INITIALIZER_GIF_EXPORT_ID:
            if (objectType != SHADRON_FLAG_EXPORT)
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            *parseContext = new ParseData { INITIALIZER_GIF_EXPORT_ID };
            *firstArgumentTypes = SHADRON_ARG_SOURCE_OBJ;
            return SHADRON_RESULT_OK;
        case INITIALIZER_QUANTIZE_ID:
            if (objectType != SHADRON_FLAG_IMAGE && objectType != SHADRON_FLAG_ANIMATION)
                return SHADRON_RESULT_UNEXPECTED_ERROR;
            *parseContext = new ParseData { INITIALIZER_QUANTIZE_ID };
            *firstArgumentTypes = SHADRON_ARG_SOURCE_OBJ;
            return SHADRON_RESULT_OK;
        default:
            return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
}

int SHADRON_API_FN shadron_parse_initializer_argument(void *context, void *parseContext, int argNo, int argumentType, const void *argumentData, int *nextArgumentTypes) {
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    switch (pd->initializer) {
        case INITIALIZER_GIF_FILE_ID:
            switch (pd->curArg) {
                case 0: // Input filename
                    if (argumentType != SHADRON_ARG_FILENAME)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->filename = reinterpret_cast<const char *>(argumentData);
                    *nextArgumentTypes = SHADRON_ARG_NONE;
                    break;
                default:
                    return SHADRON_RESULT_UNEXPECTED_ERROR;
            }
            break;
        case INITIALIZER_GIF_EXPORT_ID:
            // NOTE: In Shadron 1.4.0 and before, expression values are set after shadron_object_start_export which is too late, so they are only enabled for subsequent versions
            switch (pd->curArg) {
                case 0: // Source animation name
                    if (argumentType != SHADRON_ARG_SOURCE_OBJ)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    if (reinterpret_cast<const int *>(argumentData)[1] == SHADRON_FLAG_IMAGE)
                        pd->animated = false;
                    else if (reinterpret_cast<const int *>(argumentData)[1] == SHADRON_FLAG_ANIMATION)
                        pd->animated = true;
                    else
                        return SHADRON_RESULT_PARSE_ERROR;
                    pd->sourceId = reinterpret_cast<const int *>(argumentData)[0];
                    *nextArgumentTypes = SHADRON_ARG_FILENAME;
                    break;
                case 1: // Output filename
                    if (argumentType != SHADRON_ARG_FILENAME)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->filename = reinterpret_cast<const char *>(argumentData);
                    pd->exprFramerate = -1;
                    pd->exprDuration = -1;
                    pd->exprRepeat = -1;
                    *nextArgumentTypes = pd->animated ? (SHADRON_ARG_FLOAT|(SHADRON_VERSION >= 141 ? SHADRON_ARG_EXPR_FLOAT : 0)) : SHADRON_ARG_NONE;
                    break;
                case 2: // Animation framerate
                    if (argumentType == SHADRON_ARG_EXPR_FLOAT)
                        pd->exprFramerate = *reinterpret_cast<const int *>(argumentData);
                    else if (argumentType == SHADRON_ARG_FLOAT) {
                        pd->framerate = *reinterpret_cast<const float *>(argumentData);
                        if (pd->framerate <= 0.f)
                            return SHADRON_RESULT_PARSE_ERROR;
                    } else
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    *nextArgumentTypes = SHADRON_ARG_FLOAT|(SHADRON_VERSION >= 141 ? SHADRON_ARG_EXPR_FLOAT : 0);
                    break;
                case 3: // Animation duration
                    if (argumentType == SHADRON_ARG_EXPR_FLOAT)
                        pd->exprDuration = *reinterpret_cast<const int *>(argumentData);
                    else if (argumentType == SHADRON_ARG_FLOAT) {
                        pd->duration = *reinterpret_cast<const float *>(argumentData);
                        if (pd->duration <= 0.f)
                            return SHADRON_RESULT_PARSE_ERROR;
                    } else
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->repeat = true;
                    *nextArgumentTypes = SHADRON_ARG_NONE|SHADRON_ARG_BOOL|(SHADRON_VERSION >= 141 ? SHADRON_ARG_EXPR_BOOL : 0);
                    break;
                case 4: // Repeat (true / false)
                    if (argumentType == SHADRON_ARG_EXPR_BOOL)
                        pd->exprRepeat = *reinterpret_cast<const int *>(argumentData);
                    else if (argumentType == SHADRON_ARG_BOOL)
                        pd->repeat = *reinterpret_cast<const int *>(argumentData) != 0;
                    else
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    *nextArgumentTypes = SHADRON_ARG_NONE;
                    break;
                default:
                    return SHADRON_RESULT_UNEXPECTED_ERROR;
            }
            break;
        case INITIALIZER_QUANTIZE_ID:
            switch (pd->curArg) {
                case 0: // Source image / animation name
                    if (argumentType != SHADRON_ARG_SOURCE_OBJ)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->sourceId = reinterpret_cast<const int *>(argumentData)[0];
                    *nextArgumentTypes = SHADRON_ARG_EXPR_INT;
                    break;
                case 1: // Color count
                    if (argumentType != SHADRON_ARG_EXPR_INT)
                        return SHADRON_RESULT_UNEXPECTED_ERROR;
                    pd->exprColorCount = *reinterpret_cast<const int *>(argumentData);
                    *nextArgumentTypes = SHADRON_ARG_NONE;
                    break;
                default:
                    return SHADRON_RESULT_UNEXPECTED_ERROR;
            }
            break;
        default:
            return SHADRON_RESULT_UNEXPECTED_ERROR;
    }
    ++pd->curArg;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_parse_initializer_finish(void *context, void *parseContext, int result, int objectType, const char *objectName, int nameLength, void **object) {
    GifExtension *ext = reinterpret_cast<GifExtension *>(context);
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    int newResult = SHADRON_RESULT_OK;
    if (result == SHADRON_RESULT_OK) {
        std::string name(objectName);
        LogicalObject *obj = ext->findObject(name);
        if (obj) {
            switch (pd->initializer) {
                case INITIALIZER_GIF_FILE_ID:
                    reconfigure<GifInputObject>(obj, pd->filename);
                    break;
                case INITIALIZER_GIF_EXPORT_ID:
                    reconfigure<GifExportObject>(obj, pd->sourceId, pd->animated, pd->filename, pd->exprFramerate, pd->exprDuration, pd->exprRepeat, pd->framerate, pd->duration, pd->repeat);
                    break;
                case INITIALIZER_QUANTIZE_ID:
                    reconfigure<QuantizeObject>(obj, pd->sourceId, pd->exprColorCount);
                    break;
                default:
                    obj = NULL;
            }
        }
        if (!obj) {
            switch (pd->initializer) {
                case INITIALIZER_GIF_FILE_ID:
                    obj = new GifInputObject(name, pd->filename);
                    break;
                case INITIALIZER_GIF_EXPORT_ID:
                    obj = new GifExportObject(pd->sourceId, pd->animated, pd->filename, pd->exprFramerate, pd->exprDuration, pd->exprRepeat, pd->framerate, pd->duration, pd->repeat);
                    break;
                case INITIALIZER_QUANTIZE_ID:
                    obj = new QuantizeObject(name, pd->sourceId, pd->exprColorCount);
                    break;
                default:
                    newResult = SHADRON_RESULT_UNEXPECTED_ERROR;
            }
        }
        ext->refObject(obj);
        *object = obj;
    }
    delete pd;
    return newResult;
}

int SHADRON_API_FN shadron_parse_error_length(void *context, void *parseContext, int *length, int encoding) {
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    switch (pd->initializer) {
        case INITIALIZER_GIF_EXPORT_ID:
            switch (pd->curArg) {
                case 0: // Source animation name
                    *length = sizeof(ERROR_EXPORT_SOURCE_TYPE)-1;
                    return SHADRON_RESULT_OK;
                case 2: // Video framerate
                    if (pd->animated) {
                        *length = sizeof(ERROR_FRAMERATE_POSITIVE)-1;
                        return SHADRON_RESULT_OK;
                    }
                    break;
                case 3: // Video duration
                    if (pd->animated) {
                        *length = sizeof(ERROR_DURATION_NONNEGATIVE)-1;
                        return SHADRON_RESULT_OK;
                    }
                    break;
            }
            break;
        case INITIALIZER_QUANTIZE_ID:
            switch (pd->curArg) {
                case 1: // Color count
                    *length = sizeof(ERROR_QUANTIZE_RANGE)-1;
                    return SHADRON_RESULT_OK;
            }
            break;
        default:;
    }
    return SHADRON_RESULT_NO_DATA;
}

int SHADRON_API_FN shadron_parse_error_string(void *context, void *parseContext, void *buffer, int *length, int bufferEncoding) {
    ParseData *pd = reinterpret_cast<ParseData *>(parseContext);
    const char *errorString = NULL;
    int errorStrLen = 0;
    switch (pd->initializer) {
        case INITIALIZER_GIF_EXPORT_ID:
            switch (pd->curArg) {
                case 0: // Source animation name
                    errorString = ERROR_EXPORT_SOURCE_TYPE;
                    errorStrLen = sizeof(ERROR_EXPORT_SOURCE_TYPE)-1;
                    break;
                case 2: // Video framerate
                    if (pd->animated) {
                        errorString = ERROR_FRAMERATE_POSITIVE;
                        errorStrLen = sizeof(ERROR_FRAMERATE_POSITIVE)-1;
                    }
                    break;
                case 3: // Video duration
                    if (pd->animated) {
                        errorString = ERROR_DURATION_NONNEGATIVE;
                        errorStrLen = sizeof(ERROR_DURATION_NONNEGATIVE)-1;
                    }
                    break;
            }
            break;
        case INITIALIZER_QUANTIZE_ID:
            switch (pd->curArg) {
                case 1: // Color count
                    errorString = ERROR_QUANTIZE_RANGE;
                    errorStrLen = sizeof(ERROR_QUANTIZE_RANGE)-1;
                    break;
            }
            break;
        default:;
    }
    if (errorString) {
        if (*length < errorStrLen || bufferEncoding != SHADRON_FLAG_CHARSET_UTF8)
            return SHADRON_RESULT_UNEXPECTED_ERROR;
        memcpy(buffer, errorString, errorStrLen);
        *length = errorStrLen;
        return SHADRON_RESULT_OK;
    }
    return SHADRON_RESULT_NO_DATA;
}

int SHADRON_API_FN shadron_object_prepare(void *context, void *object, int *flags, int *width, int *height, int *format) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->prepare(*width, *height, (*flags&SHADRON_FLAG_HARD_RESET) != 0, (*flags&SHADRON_FLAG_REPEAT) != 0))
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    *flags &= SHADRON_FLAG_REPEAT;
    if (obj->acceptsFiles())
        *flags |= SHADRON_FLAG_FILE_INPUT;
    *format = SHADRON_FORMAT_RGBA_BYTE;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_size(void *context, void *object, int *width, int *height, int *format) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->getSize(*width, *height))
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    *format = SHADRON_FORMAT_RGBA_BYTE;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_load_file(void *context, void *object, const void *path, int pathLength, int pathEncoding) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->loadFile(reinterpret_cast<const char *>(path)))
        return SHADRON_RESULT_FILE_IO_ERROR;
    return SHADRON_RESULT_OK_RESIZE;
}

int SHADRON_API_FN shadron_object_unload_file(void *context, void *object) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    obj->unloadFile();
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_set_expression_value(void *context, void *object, int exprIndex, int valueType, const void *value) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    int type = -1;
    switch (valueType) {
        case SHADRON_ARG_INT:
            type = TYPE_INT;
            break;
        case SHADRON_ARG_FLOAT:
            type = TYPE_FLOAT;
            break;
        case SHADRON_ARG_BOOL:
            type = TYPE_BOOL;
            break;
    }
    int result = obj->setExpressionValue(exprIndex, type, value);
    if (result) {
        if (result == OBJ_RESIZE)
            return SHADRON_RESULT_OK_RESIZE;
        return SHADRON_RESULT_OK;
    }
    return SHADRON_RESULT_IGNORE;
}

int SHADRON_API_FN shadron_object_offer_source_pixels(void *context, void *object, int sourceIndex, int sourceType, int width, int height, int *format, void **pixelBuffer, void **pixelsContext) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    int result = obj->offerSource(*pixelBuffer, sourceIndex, width, height);
    if (result) {
        *format = SHADRON_FORMAT_RGBA_BYTE;
        if (result == OBJ_RESIZE)
            return SHADRON_RESULT_OK_RESIZE;
        return SHADRON_RESULT_OK;
    }
    return SHADRON_RESULT_IGNORE;
}

int SHADRON_API_FN shadron_object_post_source_pixels(void *context, void *object, void *pixelsContext, int sourceIndex, int plane, int width, int height, int format, const void *pixels) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (format != SHADRON_FORMAT_RGBA_BYTE)
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    obj->setSourcePixels(sourceIndex, pixels, width, height);
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_user_command(void *context, void *object, int command) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    switch (command) {
        case SHADRON_COMMAND_RESTART:
            if (obj->restart())
                return SHADRON_RESULT_OK;
        default:
            return SHADRON_RESULT_NO_CHANGE;
    }
}

int SHADRON_API_FN shadron_object_destroy(void *context, void *object) {
    GifExtension *ext = reinterpret_cast<GifExtension *>(context);
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    ext->unrefObject(obj);
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_fetch_pixels(void *context, void *object, float time, float deltaTime, int realTime, int plane, int width, int height, int format, const void **pixels, void **pixelsContext) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->pixelsReady())
        return SHADRON_RESULT_NO_DATA;
    if (!(*pixels = obj->fetchPixels(time, deltaTime, realTime != 0, width, height)))
        return SHADRON_RESULT_NO_CHANGE;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_release_pixels(void *context, void *object, void *pixelsContext) {
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_object_start_export(void *context, void *object, int *stepCount, void **exportData) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->startExport())
        return SHADRON_RESULT_NO_DATA;
    *stepCount = obj->getExportStepCount();
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_export_prepare_step(void *context, void *object, void *exportData, int step, float *time, float *deltaTime, int *outputFilenameLength, int filenameEncoding) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    const std::string *outputFilename = NULL;
    if (!obj->prepareExportStep(step, *time, *deltaTime))
        return SHADRON_RESULT_NO_MORE_ITEMS;
    if (step == 0)
        *outputFilenameLength = (int) obj->getExportFilename().size();
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_export_output_filename(void *context, void *object, void *exportData, int step, void *buffer, int *length, int encoding) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    std::string filename = obj->getExportFilename();
    if (encoding != SHADRON_FLAG_CHARSET_UTF8 || *length < (int) filename.size())
        return SHADRON_RESULT_UNEXPECTED_ERROR;
    if (filename.size() == 0)
        return SHADRON_RESULT_NO_DATA;
    memcpy(buffer, &filename[0], filename.size());
    *length = (int) filename.size();
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_export_step(void *context, void *object, void *exportData, int step, float time, float deltaTime) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    if (!obj->exportStep(step))
        return SHADRON_RESULT_FILE_IO_ERROR;
    return SHADRON_RESULT_OK;
}

int SHADRON_API_FN shadron_export_finish(void *context, void *object, void *exportData, int result) {
    LogicalObject *obj = reinterpret_cast<LogicalObject *>(object);
    obj->finishExport();
    return SHADRON_RESULT_OK;
}

}
