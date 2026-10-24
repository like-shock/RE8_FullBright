#include "runtime.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static SRWLOCK lock=SRWLOCK_INIT;
void fb_control_lock(void){AcquireSRWLockExclusive(&lock);}
void fb_control_unlock(void){ReleaseSRWLockExclusive(&lock);}
#else
#include <pthread.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
void fb_control_lock(void){pthread_mutex_lock(&lock);}
void fb_control_unlock(void){pthread_mutex_unlock(&lock);}
#endif
void fb_runtime_init(FbRuntime*r,bool supported){*r=(FbRuntime){.supported=supported};}
void fb_runtime_request(FbRuntime*r,bool on){
    if(!r->supported||r->fault)return;
    if(r->requested!=on){r->requested=on;r->applied=false;r->observation=(FbObservation){0};}
}
void fb_runtime_observe(FbRuntime*r,const FbObservation*o){
    if(!r->requested||r->fault||!r->supported||!o||!o->valid||!o->owner||!o->generation){r->applied=false;r->observation=(FbObservation){0};return;}
    if(!r->observation.valid||r->observation.owner!=o->owner||r->observation.generation!=o->generation)r->applied=false;
    r->observation=*o;
}
bool fb_runtime_note_applied(FbRuntime*r,uint64_t generation){
    if(!r->supported||!r->requested||r->fault||!r->observation.valid||r->observation.generation!=generation)return false;
    r->applied=true;return true;
}
void fb_runtime_fault(FbRuntime*r,const FbError*e){r->fault=true;r->requested=false;r->applied=false;r->observation=(FbObservation){0};if(e)r->error=*e;}
FbStatus fb_runtime_status(const FbRuntime*r){
    if(!r->supported)return FB_UNSUPPORTED_BUILD;
    if(r->fault)return FB_FAULT;
    if(!r->requested)return FB_OFF;
    return r->observation.valid&&r->applied?FB_ACTIVE:FB_WAIT_TARGET;
}
const char*fb_status_name(FbStatus status){
    switch(status){case FB_OFF:return "OFF";case FB_UNSUPPORTED_BUILD:return "UNSUPPORTED_BUILD";case FB_WAIT_TARGET:return "WAIT_TARGET";case FB_ACTIVE:return "ACTIVE";case FB_FAULT:return "FAULT";default:return "FAULT";}
}
