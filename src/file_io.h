#ifndef FB_FILE_IO_H
#define FB_FILE_IO_H
#include "common.h"
#include <stdio.h>
#include <wchar.h>
FILE *fb_file_open(const wchar_t *path, const wchar_t *mode);
bool fb_file_replace(const wchar_t *path, const void *data, size_t size, FbError *error);
#endif
