#ifndef FB_OPTIONS_H
#define FB_OPTIONS_H
#include "common.h"
#include <wchar.h>
typedef struct { unsigned schema_version; unsigned toggle_vk; bool detailed_log; } FbConfig;
typedef struct {
    FbConfig config;
    bool ready, dirty, load_error;
    int save_status; /* 1 saved/defaults, 0 pending, -1 failed */
    uint64_t dropped, io_failures;
} FbOptions;
bool fb_config_load(const wchar_t*, FbConfig*, FbError*);
bool fb_config_save(const wchar_t*, const FbConfig*, FbError*);
/* Shared options state uses the same external control lock as FbRuntime. */
void fb_options_init(const FbConfig*, bool ready);
void fb_options_load_error(bool failed);
bool fb_options_set(const FbConfig*);
FbOptions fb_options_snapshot(void);
bool fb_options_take_pending(FbConfig*, uint64_t *revision);
void fb_options_save_result(uint64_t revision, bool success);
void fb_options_diagnostics(uint64_t dropped, uint64_t io_failures);
typedef struct { unsigned vk; bool armed; } FbKeyLatch;
void fb_hotkey_arm(FbKeyLatch*, unsigned vk, bool held);
bool fb_hotkey_event(FbKeyLatch*, unsigned vk, bool down, bool repeat);
struct lua_State;
struct FbRuntime;
/* runtime.h defines the concrete runtime; no Lua API is used during detach. */
#include "runtime.h"
bool fb_ui_attach(struct lua_State*, FbRuntime*);
void fb_ui_detach(struct lua_State*);
#endif
