#ifndef FB_RUNTIME_H
#define FB_RUNTIME_H
#include "common.h"
typedef enum { FB_OFF,FB_UNSUPPORTED_BUILD,FB_WAIT_TARGET,FB_ACTIVE,FB_FAULT } FbStatus;
typedef struct { bool valid; uintptr_t owner; uint64_t generation; } FbObservation;
typedef struct { bool supported,requested,fault,applied; FbObservation observation; FbError error; } FbRuntime;
/* All access to the same runtime, including snapshots, uses this shared lock. */
void fb_control_lock(void);
void fb_control_unlock(void);
void fb_runtime_init(FbRuntime*,bool supported);
void fb_runtime_request(FbRuntime*,bool on);
void fb_runtime_observe(FbRuntime*,const FbObservation*);
bool fb_runtime_note_applied(FbRuntime*,uint64_t generation);
void fb_runtime_fault(FbRuntime*,const FbError*);
FbStatus fb_runtime_status(const FbRuntime*);
const char *fb_status_name(FbStatus);
#endif
