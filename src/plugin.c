#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <reframework/API.h>
#include "build_identity.h"
#include "profile.h"
#include "runtime.h"
#include "options.h"
#include "journal.h"
#include <stdio.h>
#include <string.h>
static const REFrameworkPluginFunctions *functions;
static FbRuntime runtime;
static FbKeyLatch key;
static bool focused = true;
static LONG initialized;
static void emit(FbStatus status, const char *reason) {
    FbEvent event = {.time_ms = GetTickCount64(), .status = status};
    snprintf(event.reason, sizeof event.reason, "%s", reason);
    fb_journal_enqueue(&event);
}
static void on_lua_created(struct lua_State *L) {
    functions->lock_lua();
    bool ok = fb_ui_attach(L, &runtime);
    functions->unlock_lua();
    if (!ok) functions->log_error("RE8 FullBright: UI attachment failed");
}
static void on_lua_destroyed(struct lua_State *L) { fb_ui_detach(L); }
static void on_present(void) {
    fb_control_lock();
    FbOptions o = fb_options_snapshot();
    if (key.vk != o.config.toggle_vk)
        fb_hotkey_arm(&key, o.config.toggle_vk, o.config.toggle_vk && (GetAsyncKeyState((int)o.config.toggle_vk) & 0x8000));
    fb_control_unlock();
}
static bool on_message(void *hwnd, unsigned int msg, unsigned long long wp, long long lp) {
    (void)hwnd;
    bool changed = false;
    fb_control_lock();
    FbOptions o = fb_options_snapshot();
    if (msg == WM_KILLFOCUS || (msg == WM_ACTIVATEAPP && !wp)) {
        focused = false; fb_hotkey_arm(&key, o.config.toggle_vk, true);
    } else if (msg == WM_SETFOCUS || (msg == WM_ACTIVATEAPP && wp)) {
        focused = true;
        fb_hotkey_arm(&key, o.config.toggle_vk, o.config.toggle_vk && (GetAsyncKeyState((int)o.config.toggle_vk) & 0x8000));
    } else if (focused && o.ready && key.vk == o.config.toggle_vk &&
               (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYUP)) {
        bool down = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        if (fb_hotkey_event(&key, (unsigned)wp, down, ((unsigned long long)lp & (1ull << 30)) != 0)) {
            bool before = runtime.requested;
            fb_runtime_request(&runtime, !before);
            changed = runtime.requested != before;
        }
    }
    FbStatus status = fb_runtime_status(&runtime);
    fb_control_unlock();
    if (changed) emit(status, "hotkey_request");
    return true; /* Do not consume game input. */
}
static bool directory(const wchar_t *path) {
    if (CreateDirectoryW(path, NULL)) return true;
    DWORD attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}
static bool data_paths(const wchar_t *exe, wchar_t *dir, wchar_t *config) {
    const wchar_t *slash = wcsrchr(exe, L'\\');
    if (!slash || (size_t)(slash-exe) > 32600) return false;
    size_t root = (size_t)(slash-exe);
    wmemcpy(dir, exe, root); dir[root] = 0;
    const wchar_t *parts[] = {L"\\reframework", L"\\data", L"\\RE8_FullBright"};
    for (unsigned i = 0; i < 3; ++i) { wcscat(dir, parts[i]); if (!directory(dir)) return false; }
    return swprintf(config, 32768, L"%ls\\config.ini", dir) > 0;
}
static DWORD WINAPI worker_main(void *unused) {
    (void)unused;
    wchar_t exe[32768], dir[32768], config_path[32768];
    DWORD length = GetModuleFileNameW(NULL, exe, 32768);
    FbError error = {0}; FbConfig config = {1, 0, false};
    bool have_path = length && length < 32768;
    bool storage = have_path && data_paths(exe, dir, config_path);
    bool loaded = storage && fb_config_load(config_path, &config, &error);
    bool logging = storage && fb_journal_start(dir, &error);
    if (!storage || !loaded || !logging) functions->log_error("RE8 FullBright: settings/log storage unavailable or invalid; defaults/errors shown in menu");
    fb_control_lock();
    fb_options_init(&config, true); fb_options_load_error(!loaded);
    fb_control_unlock();
    FbBuildIdentity identity = {0};
    if (have_path && fb_identify_file(exe, &identity, &error)) {
        char hash[65]; static const char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) { hash[2*i] = digits[identity.sha256[i]>>4]; hash[2*i+1] = digits[identity.sha256[i]&15]; } hash[64] = 0;
        /* Recognition alone MUST NOT enable a future profile: live-image validation
           and hook installation are subsequent tasks. Foundation stays unsupported. */
        functions->log_info("RE8 FullBright: exe_sha256=%s image_size=%u; UNSUPPORTED_BUILD (no hooks)", hash, identity.image_size);
    } else functions->log_error("RE8 FullBright: executable identity unavailable: %s", error.message);
    emit(FB_UNSUPPORTED_BUILD, "foundation_no_profiles");
    FbStatus previous = FB_UNSUPPORTED_BUILD;
    uint64_t last_sample = 0;
    /* DLL and callbacks have process lifetime; no unsafe unload path is exposed. */
    for (;;) {
        FbConfig pending; uint64_t revision = 0;
        FbJournalStats stats = fb_journal_stats();
        fb_control_lock();
        bool save = fb_options_take_pending(&pending, &revision);
        FbOptions o = fb_options_snapshot();
        FbStatus status = fb_runtime_status(&runtime);
        fb_options_diagnostics(stats.dropped, stats.io_failures + (!logging && !stats.io_failures ? 1 : 0));
        fb_control_unlock();
        if (save) {
            bool ok = storage && fb_config_save(config_path, &pending, &error);
            fb_control_lock(); fb_options_save_result(revision, ok); fb_control_unlock();
            emit(status, ok ? "config_saved" : "config_save_failed");
        }
        if (status != previous) { emit(status, "state_changed"); previous = status; }
        uint64_t now = GetTickCount64();
        if (o.config.detailed_log && now-last_sample >= 1000) { emit(status, "periodic_status"); last_sample = now; }
        fb_journal_drain();
        Sleep(250);
    }
    return 0;
}
__declspec(dllexport) void reframework_plugin_required_version(REFrameworkPluginVersion *v) {
    if (v) *v = (REFrameworkPluginVersion){REFRAMEWORK_PLUGIN_VERSION_MAJOR, REFRAMEWORK_PLUGIN_VERSION_MINOR, REFRAMEWORK_PLUGIN_VERSION_PATCH, "RE8"};
}
__declspec(dllexport) bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam *p) {
    if (!p || !p->version || !p->functions || p->version->major != REFRAMEWORK_PLUGIN_VERSION_MAJOR || p->version->minor < REFRAMEWORK_PLUGIN_VERSION_MINOR) return false;
    functions = p->functions;
    if (!functions->log_error || !functions->log_info || !functions->on_lua_state_created || !functions->on_lua_state_destroyed ||
        !functions->lock_lua || !functions->unlock_lua || !functions->on_message || !functions->on_present) return false;
    if (InterlockedCompareExchange(&initialized, 1, 0)) return false;
    FbConfig config = {1, 0, false};
    fb_control_lock(); fb_runtime_init(&runtime, false); fb_options_init(&config, false); fb_control_unlock();
    HMODULE self = NULL;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          (LPCWSTR)&reframework_plugin_initialize, &self)) return false;
    if (!functions->on_lua_state_created(on_lua_created) || !functions->on_lua_state_destroyed(on_lua_destroyed) ||
        !functions->on_message(on_message) || !functions->on_present(on_present)) {
        functions->log_error("RE8 FullBright: callback registration failed"); return false;
    }
    functions->log_info("RE8 FullBright: requested OFF; foundation build; no supported game profiles");
    HANDLE worker = CreateThread(NULL, 0, worker_main, NULL, 0, NULL);
    if (!worker) { functions->log_error("RE8 FullBright: worker unavailable"); return false; }
    CloseHandle(worker); return true;
}
