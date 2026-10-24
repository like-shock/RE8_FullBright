#ifndef FB_BUILD_IDENTITY_H
#define FB_BUILD_IDENTITY_H
#include "common.h"
#include <wchar.h>
typedef struct { uint8_t sha256[32]; uint16_t machine; uint32_t timestamp,image_size; } FbBuildIdentity;
bool fb_parse_pe(const uint8_t *data,size_t length,FbBuildIdentity *out,FbError *error);
bool fb_identify_file(const wchar_t *path,FbBuildIdentity *out,FbError *error);
#endif
