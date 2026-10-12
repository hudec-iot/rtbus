/* SPDX-License-Identifier: MPL-2.0 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
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
    FILE *file;
    int result;
    char *args[] = { NULL, "-device", NULL, "-if", "SWD", "-speed", NULL,
                    "-IP", NULL, "-AutoConnect", "1", "-NoGui", "1",
                    "-ExitOnError", "1", "-CommanderScript", script, NULL };
    if (argc != 8 || (strcmp(argv[1], "check") && strcmp(argv[1], "write"))) {
        fprintf(stderr, "Usage: jlink_bootloader <check|write> <client> <device> <ip:port> <speed-kHz> <bootloader.hex> <runtime.hex>\n");
        return 1;
    }
    if (!readable(argv[2]) || !readable(argv[6]) || !readable(argv[7])) return 1;
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
    fputs("ExitOnError 1\nr\nh\n", file);
    result = loadfile(file, argv[6]) && loadfile(file, argv[7]);
    fputs("r\ng\nExit\n", file);
    if (fclose(file)) result = 0;
    if (result) {
        args[0] = argv[2]; args[2] = argv[3];
        args[6] = argv[5]; args[8] = argv[4];
        result = run(args);
    } else result = 1;
    remove(script);
    return result;
}
