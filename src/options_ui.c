#include "options.h"
#include "lua.h"
#include "lauxlib.h"
#include <stdio.h>
static lua_State *attached;
static FbRuntime *target;
static uint64_t generation;
/* REFramework serializes Lua callbacks. The control lock also covers native inputs. */
static bool live(lua_State *L) {
    return attached == L && target && (uint64_t)lua_tointeger(L, lua_upvalueindex(1)) == generation;
}
static void boolean(lua_State *L, const char *name, bool value) { lua_pushboolean(L, value); lua_setfield(L, -2, name); }
static void integer(lua_State *L, const char *name, lua_Integer value) { lua_pushinteger(L, value); lua_setfield(L, -2, name); }
static int snapshot(lua_State *L) {
    fb_control_lock();
    if (!live(L)) { fb_control_unlock(); lua_pushnil(L); return 1; }
    FbRuntime r = *target; FbOptions o = fb_options_snapshot(); fb_control_unlock();
    lua_newtable(L);
    boolean(L, "requested", r.requested); boolean(L, "supported", r.supported && !r.fault);
    boolean(L, "ready", o.ready); boolean(L, "detailed", o.config.detailed_log); boolean(L, "load_error", o.load_error);
    integer(L, "toggle", o.config.toggle_vk ? (lua_Integer)o.config.toggle_vk - 110 : 1);
    integer(L, "save_status", o.save_status);
    lua_pushstring(L, fb_status_name(fb_runtime_status(&r))); lua_setfield(L, -2, "status");
    char counters[128]; snprintf(counters, sizeof counters, "Log dropped: %llu; I/O errors: %llu", (unsigned long long)o.dropped, (unsigned long long)o.io_failures);
    lua_pushstring(L, counters); lua_setfield(L, -2, "diagnostics"); return 1;
}
static int request(lua_State *L) {
    bool on = lua_isboolean(L, 1) && lua_toboolean(L, 1);
    fb_control_lock();
    if (live(L) && fb_options_snapshot().ready) fb_runtime_request(target, on);
    fb_control_unlock(); return 0;
}
static int configure(lua_State *L) {
    if (!lua_isinteger(L, 1) || !lua_isboolean(L, 2)) return 0;
    lua_Integer index = lua_tointeger(L, 1);
    if (index < 1 || index > 13) return 0;
    bool detailed = lua_toboolean(L, 2);
    fb_control_lock();
    if (live(L)) { FbConfig c = {1, index > 1 ? (unsigned)index + 110 : 0, detailed}; fb_options_set(&c); }
    fb_control_unlock(); return 0;
}
static const char script[] =
    "local api = ...\n"
    "re.on_draw_ui(function()\n"
    " local s=api.get(); if not s then return end\n"
    " if not imgui.tree_node('RE8 FullBright') then return end\n"
    " imgui.text('Status: '..s.status)\n"
    " if not s.ready then imgui.text('Loading settings...')\n"
    " else\n"
    "  if s.supported then local changed,on=imgui.checkbox('Fullbright',s.requested); if changed then api.request(on) end\n"
    "  else imgui.text('No verified RE8 profile. Fullbright unavailable.') end\n"
    "  local changed,key=imgui.combo('Toggle key',s.toggle,{'None','F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','F11','F12'})\n"
    "  local changed_log,detailed=imgui.checkbox('Detailed diagnostics',s.detailed)\n"
    "  if changed or changed_log then api.configure(key,detailed) end\n"
    "  imgui.text('Steam Input: map a rear button to the selected key.')\n"
    "  if s.load_error then imgui.text('Config invalid/unreadable: defaults loaded.') end\n"
    "  if s.save_status < 0 then imgui.text('Config save failed. Change a setting to retry.') end\n"
    "  imgui.text(s.diagnostics)\n"
    " end\n"
    " imgui.tree_pop()\n"
    "end)\n";
static bool has_function(lua_State *L, const char *table, const char *field) {
    lua_getglobal(L, table);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }
    lua_getfield(L, -1, field); bool ok = lua_isfunction(L, -1); lua_pop(L, 2); return ok;
}
bool fb_ui_attach(lua_State *L, FbRuntime *runtime) {
    if (!L || !runtime) return false;
    fb_control_lock(); bool exists = attached == L; fb_control_unlock();
    if (exists) return true;
    const char *fields[] = {"tree_node", "tree_pop", "text", "checkbox", "combo"};
    if (!has_function(L, "re", "on_draw_ui")) return false;
    for (unsigned i = 0; i < sizeof fields / sizeof fields[0]; ++i) if (!has_function(L, "imgui", fields[i])) return false;
    int top = lua_gettop(L);
    if (luaL_loadbuffer(L, script, sizeof script - 1, "RE8_FullBright_UI") != LUA_OK) { lua_settop(L, top); return false; }
    fb_control_lock(); attached = L; target = runtime; ++generation; uint64_t token = generation; fb_control_unlock();
    lua_newtable(L);
    const luaL_Reg methods[] = {{"get",snapshot},{"request",request},{"configure",configure},{NULL,NULL}};
    lua_pushinteger(L, (lua_Integer)token); luaL_setfuncs(L, methods, 1);
    bool ok = lua_pcall(L, 1, 0, 0) == LUA_OK;
    lua_settop(L, top);
    if (!ok) fb_ui_detach(L);
    return ok;
}
void fb_ui_detach(lua_State *L) {
    fb_control_lock();
    if (attached == L) { attached = NULL; target = NULL; ++generation; }
    fb_control_unlock();
}
