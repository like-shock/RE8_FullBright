#include "options.h"
#include "file_io.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
static const FbConfig defaults = {1, 0, false};
static FbOptions options;
static uint64_t revision;
static bool valid(const FbConfig *c) {
    return c && c->schema_version == 1 && (c->toggle_vk == 0 || (c->toggle_vk >= 112 && c->toggle_vk <= 123));
}
bool fb_config_load(const wchar_t *path, FbConfig *out, FbError *error) {
    *out = defaults;
    FILE *file = fb_file_open(path, L"rb");
    if (!file) {
        if (errno == ENOENT) return true;
        fb_error(error, 1, "config cannot be read"); return false;
    }
    FbConfig c = defaults;
    unsigned seen = 0;
    char line[128];
    bool ok = true;
    while (fgets(line, sizeof line, file)) {
        size_t n = strlen(line);
        if (n == sizeof line - 1 && line[n-1] != '\n') { ok = false; break; }
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (!n || line[0] == '#' || line[0] == ';') continue;
        char *equals = strchr(line, '=');
        if (!equals) { ok = false; break; }
        *equals++ = 0;
        if (!*equals || strspn(equals, "0123456789") != strlen(equals)) { ok = false; break; }
        char *end;
        errno = 0;
        unsigned long value = strtoul(equals, &end, 10);
        if (errno || *end || value > 999) { ok = false; break; }
        unsigned bit = 0;
        if (!strcmp(line, "schema_version")) { bit = 1; c.schema_version = (unsigned)value; }
        else if (!strcmp(line, "toggle_vk")) { bit = 2; c.toggle_vk = (unsigned)value; }
        else if (!strcmp(line, "detailed_log") && value <= 1) { bit = 4; c.detailed_log = value != 0; }
        else { ok = false; break; }
        if (seen & bit) { ok = false; break; }
        seen |= bit;
    }
    if (ferror(file)) ok = false;
    if (fclose(file)) ok = false;
    if (!ok || !(seen & 1) || !valid(&c)) {
        fb_error(error, 2, "invalid config; defaults loaded"); return false;
    }
    *out = c; return true;
}
bool fb_config_save(const wchar_t *path, const FbConfig *config, FbError *error) {
    if (!valid(config)) { fb_error(error, 3, "invalid config; save refused"); return false; }
    char data[128];
    int n = snprintf(data, sizeof data, "schema_version=1\ntoggle_vk=%u\ndetailed_log=%u\n", config->toggle_vk, config->detailed_log ? 1u : 0u);
    return n > 0 && (size_t)n < sizeof data && fb_file_replace(path, data, (size_t)n, error);
}
void fb_options_init(const FbConfig *config, bool ready) {
    options = (FbOptions){.config = valid(config) ? *config : defaults, .ready = ready, .save_status = 1};
    ++revision;
}
void fb_options_load_error(bool failed) { options.load_error = failed; }
FbOptions fb_options_snapshot(void) { return options; }
bool fb_options_set(const FbConfig *config) {
    if (!options.ready || !valid(config)) return false;
    if (config->toggle_vk == options.config.toggle_vk && config->detailed_log == options.config.detailed_log) return true;
    options.config = *config; options.dirty = true; options.save_status = 0; ++revision;
    return true;
}
bool fb_options_take_pending(FbConfig *out, uint64_t *rev) {
    if (!options.ready || !options.dirty || options.save_status == -1) return false;
    *out = options.config; *rev = revision; return true;
}
void fb_options_save_result(uint64_t rev, bool success) {
    if (rev != revision) return;
    options.save_status = success ? 1 : -1;
    if (success) { options.dirty = false; options.load_error = false; }
}
void fb_options_diagnostics(uint64_t dropped, uint64_t failures) { options.dropped = dropped; options.io_failures = failures; }
void fb_hotkey_arm(FbKeyLatch *key, unsigned vk, bool held) { *key = (FbKeyLatch){vk, !held}; }
bool fb_hotkey_event(FbKeyLatch *key, unsigned vk, bool down, bool repeat) {
    if (!key->vk || vk != key->vk) return false;
    if (!down) { key->armed = true; return false; }
    bool toggle = key->armed && !repeat;
    key->armed = false;
    return toggle;
}
