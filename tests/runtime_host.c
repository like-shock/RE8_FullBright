#include "runtime.h"
#include <assert.h>
#include <pthread.h>
static FbRuntime r;
static void *exercise(void*p){
    (void)p;
    for(unsigned i=1;i<2000;i++){
        fb_control_lock();fb_runtime_request(&r,true);
        FbObservation o={true,123,i};fb_runtime_observe(&r,&o);
        assert(fb_runtime_note_applied(&r,i));assert(fb_runtime_status(&r)==FB_ACTIVE);
        fb_runtime_request(&r,false);assert(!fb_runtime_note_applied(&r,i));
        assert(fb_runtime_status(&r)==FB_OFF);fb_control_unlock();
    }return NULL;
}
int main(void){
    fb_control_lock();
    fb_runtime_init(&r,false);fb_runtime_request(&r,true);assert(fb_runtime_status(&r)==FB_UNSUPPORTED_BUILD);
    fb_runtime_init(&r,true);assert(fb_runtime_status(&r)==FB_OFF);
    fb_runtime_request(&r,true);assert(fb_runtime_status(&r)==FB_WAIT_TARGET);
    FbObservation a={true,123,7};fb_runtime_observe(&r,&a);
    assert(fb_runtime_status(&r)==FB_WAIT_TARGET);assert(!fb_runtime_note_applied(&r,6));
    assert(fb_runtime_note_applied(&r,7));assert(fb_runtime_status(&r)==FB_ACTIVE);
    a.generation=8;fb_runtime_observe(&r,&a);assert(!fb_runtime_note_applied(&r,7));
    assert(fb_runtime_status(&r)==FB_WAIT_TARGET);
    fb_runtime_note_applied(&r,8);a.valid=false;fb_runtime_observe(&r,&a);
    assert(fb_runtime_status(&r)==FB_WAIT_TARGET);assert(!fb_runtime_note_applied(&r,8));
    a.valid=true;a.owner=0;fb_runtime_observe(&r,&a);assert(!fb_runtime_note_applied(&r,8));
    a.owner=123;fb_runtime_observe(&r,&a);fb_runtime_request(&r,false);
    assert(!fb_runtime_note_applied(&r,8));fb_runtime_request(&r,true);
    assert(fb_runtime_status(&r)==FB_WAIT_TARGET);assert(!fb_runtime_note_applied(&r,8));
    FbError err={1,"read_failed"};fb_runtime_fault(&r,&err);fb_runtime_request(&r,true);
    assert(fb_runtime_status(&r)==FB_FAULT);fb_runtime_init(&r,true);fb_control_unlock();
    pthread_t threads[4];for(unsigned i=0;i<4;i++)assert(!pthread_create(&threads[i],NULL,exercise,NULL));
    for(unsigned i=0;i<4;i++)assert(!pthread_join(threads[i],NULL));
    return 0;
}
