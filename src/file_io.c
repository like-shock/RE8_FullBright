#define _POSIX_C_SOURCE 200809L
#include "file_io.h"
#include <errno.h>
#include <stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
static char *narrow(const wchar_t *path) {
    size_t size = wcstombs(NULL, path, 0);
    if (size == (size_t)-1) { errno = EINVAL; return NULL; }
    char *result = malloc(size + 1);
    if (result) wcstombs(result, path, size + 1);
    return result;
}
#endif
FILE *fb_file_open(const wchar_t *path, const wchar_t *mode) {
#ifdef _WIN32
    return _wfopen(path, mode);
#else
    char *p = narrow(path), *m = narrow(mode);
    FILE *file = p && m ? fopen(p, m) : NULL;
    int saved = errno;
    free(p); free(m); errno = saved;
    return file;
#endif
}
bool fb_file_replace(const wchar_t *path, const void *data, size_t size, FbError *error) {
    /* Exclusive sibling creation; a stale temp can never be overwritten. */
    size_t length = wcslen(path);
    wchar_t *temp = malloc((length + 48) * sizeof *temp);
    if (!temp) { fb_error(error, 1, "config allocation failed"); return false; }
    FILE *file = NULL;
    for (unsigned attempt = 0; attempt < 100 && !file; ++attempt) {
#ifdef _WIN32
        swprintf(temp, length + 48, L"%ls.tmp.%lu.%u", path, (unsigned long)GetCurrentProcessId(), attempt);
        HANDLE handle = CreateFileW(temp, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_EXISTS) continue;
            break;
        }
        int fd = _open_osfhandle((intptr_t)handle, _O_BINARY | _O_WRONLY);
        if (fd == -1) { CloseHandle(handle); DeleteFileW(temp); break; }
        file = _fdopen(fd, "wb");
        if (!file) { _close(fd); DeleteFileW(temp); break; }
#else
        swprintf(temp, length + 48, L"%ls.tmp.%lu.%u", path, (unsigned long)getpid(), attempt);
        char *p = narrow(temp);
        if (!p) break;
        int fd = open(p, O_WRONLY | O_CREAT | O_EXCL, 0600);
        int saved = errno;
        free(p);
        if (fd == -1) { if (saved == EEXIST) continue; break; }
        file = fdopen(fd, "wb");
        if (!file) { close(fd); p = narrow(temp); if (p) unlink(p); free(p); break; }
#endif
    }
    bool ok = file != NULL;
    if (file) {
        ok = fwrite(data, 1, size, file) == size && fflush(file) == 0;
#ifdef _WIN32
        if (ok) ok = _commit(_fileno(file)) == 0;
#else
        if (ok) ok = fsync(fileno(file)) == 0;
#endif
        if (fclose(file) != 0) ok = false;
#ifdef _WIN32
        if (ok) ok = MoveFileExW(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        if (!ok) DeleteFileW(temp);
#else
        char *src = narrow(temp), *dst = narrow(path);
        if (ok) ok = src && dst && rename(src, dst) == 0;
        if (!ok && src) unlink(src);
        free(src); free(dst);
#endif
    }
    free(temp);
    if (!ok) fb_error(error, 2, "config save failed; previous file retained");
    return ok;
}
