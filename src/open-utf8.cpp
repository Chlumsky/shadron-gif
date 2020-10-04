
#define _CRT_SECURE_NO_WARNINGS

#include "open-utf8.h"

#ifdef _WIN32

#include <cstdlib>
#include <io.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static LPWSTR utf8ToWStr(const char *str) {
    LPWSTR wstr = NULL;
    int utf8Len = (int) strlen(str);
    if (utf8Len > 0) {
        int buffLen = utf8Len+1;
        wstr = reinterpret_cast<LPWSTR>(malloc(sizeof(WCHAR)*buffLen));
        int wLen = MultiByteToWideChar(CP_UTF8, 0, str, utf8Len, wstr, buffLen);
        if (wLen < 0 || wLen >= buffLen) {
            free(wstr);
            wstr = NULL;
        }
        wstr[wLen] = (WCHAR) 0;
    }
    return wstr;
}

int openUtf8(const char *utf8Filename, int oflag) {
    int fd = -1;
    LPWSTR wFilename = utf8ToWStr(utf8Filename);
    if (wFilename) {
        fd = _wopen(wFilename, oflag);
        free(wFilename);
    }
    return fd;
}

int openUtf8(const char *utf8Filename, int oflag, int pmode) {
    int fd = -1;
    LPWSTR wFilename = utf8ToWStr(utf8Filename);
    if (wFilename) {
        fd = _wopen(wFilename, oflag, pmode);
        free(wFilename);
    }
    return fd;
}

#else

#include <unistd.h>

int openUtf8(const char *utf8Filename, int oflag) {
    return open(utf8Filename, oflag);
}

int openUtf8(const char *utf8Filename, int oflag, int pmode) {
    return open(utf8Filename, oflag, pmode);
}

#endif
