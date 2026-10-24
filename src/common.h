#ifndef FB_COMMON_H
#define FB_COMMON_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { int code; char message[96]; } FbError;
static inline bool fb_error(FbError *error,int code,const char *message) {
    if(error) { size_t i=0;error->code=code;
        for(;i+1<sizeof error->message && message[i];i++)error->message[i]=message[i];
        error->message[i]=0;
    }
    return false;
}
#endif
