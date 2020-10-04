
#pragma once

#include <fcntl.h>

int openUtf8(const char *utf8Filename, int oflag);
int openUtf8(const char *utf8Filename, int oflag, int pmode);
