/* SPDX-License-Identifier: MPL-2.0 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "framework/abi/include/rtbus_image_format.h"
#define RTBUS_KEY_DERIVE_ONLY
#include "crypto/signing.c"
#include "key/key_store.c"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#ifdef _WIN32
static int registry_client(HKEY key, char *client, size_t capacity)
{
    char value[4096], directory[4096];
    DWORD type = 0, bytes = sizeof(value);
    size_t length;
    int written;
    DWORD attributes;
    if (RegQueryValueExA(key, "InstallPath", NULL, &type,
                         (BYTE *)value, &bytes) != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) || !bytes ||
        bytes > sizeof(value)) return 0;
    value[sizeof(value) - 1] = 0;
    if (type == REG_EXPAND_SZ) {
        DWORD expanded = ExpandEnvironmentStringsA(value, directory, sizeof(directory));
        if (!expanded || expanded > sizeof(directory)) return 0;
    } else {
        memcpy(directory, value, bytes);
        directory[bytes - 1] = 0;
    }
    length = strlen(directory);
    while (length && (directory[length - 1] == '\\' || directory[length - 1] == '/'))
        directory[--length] = 0;
    if (!length) return 0;
    written = snprintf(client, capacity, "%s\\JLink.exe", directory);
    if (written < 0 || (size_t)written >= capacity) return 0;
    attributes = GetFileAttributesA(client);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static int find_registered_client(char *client, size_t capacity)
{
    HKEY hives[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    REGSAM views[] = { KEY_WOW64_64KEY, KEY_WOW64_32KEY };
    size_t hive, view;
    for (hive = 0; hive < sizeof(hives) / sizeof(hives[0]); ++hive) {
        for (view = 0; view < sizeof(views) / sizeof(views[0]); ++view) {
            HKEY key;
            DWORD index = 0;
            if (RegOpenKeyExA(hives[hive], "Software\\SEGGER\\J-Link", 0,
                              KEY_READ | views[view], &key) != ERROR_SUCCESS) continue;
            if (registry_client(key, client, capacity)) { RegCloseKey(key); return 1; }
            for (;;) {
                char name[256];
                DWORD length = sizeof(name);
                HKEY version;
                LONG status = RegEnumKeyExA(key, index++, name, &length,
                                            NULL, NULL, NULL, NULL);
                if (status == ERROR_NO_MORE_ITEMS) break;
                if (status != ERROR_SUCCESS) break;
                if (RegOpenKeyExA(key, name, 0, KEY_READ | views[view], &version)
                    != ERROR_SUCCESS) continue;
                if (registry_client(version, client, capacity)) {
                    RegCloseKey(version); RegCloseKey(key); return 1;
                }
                RegCloseKey(version);
            }
            RegCloseKey(key);
        }
    }
    fprintf(stderr, "No registered J-Link installation with JLink.exe found. Install SEGGER J-Link Software and Documentation Pack.\n");
    return 0;
}
#endif

static int readable(const char *path)
{
    FILE *file = fopen(path, "rb");
    int ok;
    if (!file) {
        fprintf(stderr, "Missing file: %s\n", path);
        return 0;
    }
    ok = fgetc(file) != EOF;
    fclose(file);
    if (!ok) fprintf(stderr, "Empty file: %s\n", path);
    return ok;
}

static int loadfile(FILE *file, const char *path)
{
    const unsigned char *p = (const unsigned char *)path;
    /* Use forward slashes on Windows too; reject script command injection. */
    for (; *p; ++p)
        if (*p == '"' || *p == '\n' || *p == '\r') return 0;
    fputs("loadfile \"", file);
    for (p = (const unsigned char *)path; *p; ++p)
        fputc(*p == '\\' ? '/' : *p, file);
    fputs("\"\n", file);
    return 1;
}

static int run(char **args)
{
#ifdef _WIN32
    char command[32768], *out = command;
    size_t remaining = sizeof(command);
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    DWORD code;
    int i;
    /* Quote according to the Windows argv rules, including trailing slashes. */
    for (i = 0; args[i]; ++i) {
        const char *p = args[i];
        size_t slashes = 0;
        if (strlen(p) * 2 + 4 >= remaining) return 1;
        *out++ = '"';
        while (*p) {
            if (*p == '\\') { ++slashes; ++p; continue; }
            if (*p == '"') {
                size_t n = slashes * 2 + 1;
                while (n--) *out++ = '\\';
            } else {
                while (slashes) { *out++ = '\\'; --slashes; }
            }
            slashes = 0;
            *out++ = *p++;
        }
        while (slashes) { *out++ = '\\'; *out++ = '\\'; --slashes; }
        *out++ = '"'; *out++ = ' ';
        remaining = sizeof(command) - (size_t)(out - command);
    }
    *out = 0;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    if (!CreateProcessA(args[0], command, NULL, NULL, TRUE, 0, NULL, NULL,
                        &startup, &process)) {
        fprintf(stderr, "Cannot start J-Link client (Windows error %lu)\n",
                (unsigned long)GetLastError());
        return 1;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    if (!GetExitCodeProcess(process.hProcess, &code)) code = 1;
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    return code == 0 ? 0 : 1;
#else
    int status;
    pid_t pid = fork();
    if (pid < 0) return 1;
    if (!pid) { execv(args[0], args); perror("J-Link client"); _exit(127); }
    if (waitpid(pid, &status, 0) < 0) return 1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif
}

int main(int argc, char **argv)
{
    char script[4096];
    char key_hex[4110];
    const char *private_path;
    uint8_t private_key[32], public_key[65];
    uint32_t key_address, key_size;
    unsigned long parsed;
    char *end;
    char *client;
#ifdef _WIN32
    char registered_client[4096];
#endif
    FILE *file;
    int result;
    char *args[] = { NULL, "-device", NULL, "-if", "SWD", "-speed", NULL,
                    "-IP", NULL, "-AutoConnect", "1", "-NoGui", "1",
                    "-ExitOnError", "1", "-CommanderScript", script, NULL };
    if (argc != 10 || (strcmp(argv[1], "check") && strcmp(argv[1], "write"))) {
        fprintf(stderr, "Usage: jlink_runtime <check|write> <client> <device> <ip:port> <speed-kHz> <runtime.hex> <private.key> <key-address> <key-partition-size>\n");
        return 1;
    }
    errno = 0;
    parsed = strtoul(argv[8], &end, 0);
    if (errno || end == argv[8] || *end || parsed > UINT32_MAX) return 1;
    key_address = (uint32_t)parsed;
    errno = 0;
    parsed = strtoul(argv[9], &end, 0);
    if (errno || end == argv[9] || *end || parsed > UINT32_MAX) return 1;
    key_size = (uint32_t)parsed;
    if (!key_address || key_size < RTBUS_KEY_PUBLIC_SIZE || key_address % key_size) return 1;
    private_path = getenv("RTBUS_SIGNING_KEY");
    if (!private_path || !*private_path) private_path = argv[7];
    if (rtbus_read_private_key(private_path, private_key) != 0 ||
        rtbus_public_key(private_key, public_key) != 0 ||
        validate_runtime_hex(argv[6], (uint64_t)key_address + key_size) != 0) return 1;
    client = argv[2];
#ifdef _WIN32
    if (!strcmp(client, "registry")) {
        if (!find_registered_client(registered_client, sizeof(registered_client))) return 1;
        client = registered_client;
    }
#endif
    if (!readable(client) || !readable(argv[6])) return 1;
    printf("J-Link client: %s\n", client);
    fflush(stdout);
    /* Arduino's erase phase validates everything without a whole-chip erase. */
    if (!strcmp(argv[1], "check")) return 0;
#ifdef _WIN32
    {
        char temp[MAX_PATH];
        DWORD size = GetTempPathA(sizeof(temp), temp);
        if (!size || size >= sizeof(temp) || !GetTempFileNameA(temp, "rtb", 0, script)) return 1;
        file = fopen(script, "w");
    }
#else
    {
        int fd;
        strcpy(script, "/tmp/rtbus-jlink-XXXXXX");
        fd = mkstemp(script);
        if (fd < 0) return 1;
        file = fdopen(fd, "w");
        if (!file) close(fd);
    }
#endif
    if (!file) { remove(script); return 1; }
    snprintf(key_hex, sizeof(key_hex), "%s.key.hex", script);
    if (write_key_hex(key_hex, key_address, public_key) != 0) {
        fclose(file); remove(script); remove(key_hex); return 1;
    }
    fputs("ExitOnError 1\nr\nh\n", file);
    result = loadfile(file, argv[6]);
    if (result) result = loadfile(file, key_hex);
    fputs("r\ng\nExit\n", file);
    if (fclose(file)) result = 0;
    if (result) {
        args[0] = client; args[2] = argv[3];
        args[6] = argv[5]; args[8] = argv[4];
        result = run(args);
    } else result = 1;
    remove(script);
    remove(key_hex);
    return result;
}
