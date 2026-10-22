#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shobjidl.h>
#include <stdio.h>
#include <wchar.h>
#include <stdbool.h>

#define JAR_NAME L"Raptor-Release.jar"
#define RAPTOR_APP_ID L"Doper.Raptor"

static void log_error(const wchar_t* message) {
    fwprintf(stderr, L"[ERROR] %ls\n", message);
    fflush(stderr);
}

static bool file_exists(const wchar_t* path) {
    DWORD a = GetFileAttributesW(path);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static bool join_path(wchar_t* out, size_t cap, const wchar_t* a, const wchar_t* b) {
    return swprintf_s(out, cap, L"%ls%ls%ls", a,
        (wcslen(a) && a[wcslen(a)-1] == L'\\') ? L"" : L"\\", b) >= 0;
}

static bool copy_if_file(wchar_t* out, size_t cap, const wchar_t* candidate) {
    if (file_exists(candidate) && wcscpy_s(out, cap, candidate) == 0) return true;
    return false;
}

static bool find_java(const wchar_t* app_dir, wchar_t* out, size_t cap) {
    wchar_t candidate[32768];

    // Prefer the bundled runtime: the packaged app then needs no system Java.
    if (join_path(candidate, _countof(candidate), app_dir, L"runtime\\bin\\java.exe") &&
        copy_if_file(out, cap, candidate)) {
        wprintf(L"[OK] Using bundled Java: %ls\n", out);
        return true;
    }

    wprintf(L"[INFO] Bundled runtime not found; checking JAVA_HOME and PATH.\n");
    wchar_t home[32768];
    DWORD n = GetEnvironmentVariableW(L"JAVA_HOME", home, (DWORD)_countof(home));
    if (n > 0 && n < _countof(home)) {
        wprintf(L"[INFO] JAVA_HOME = %ls\n", home);
        if (join_path(candidate, _countof(candidate), home, L"bin\\java.exe") &&
            copy_if_file(out, cap, candidate)) {
            wprintf(L"[OK] Found Java: %ls\n", out);
            return true;
        }
    }

    DWORD found = SearchPathW(NULL, L"java.exe", NULL, (DWORD)cap, out, NULL);
    if (found > 0 && found < cap && file_exists(out)) {
        wprintf(L"[OK] Found Java on PATH: %ls\n", out);
        return true;
    }
    return false;
}

int wmain(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    wprintf(L"========================================\n  RAPTOR ENGINE LAUNCHER (Windows x64)\n========================================\n\n");

    HRESULT hr = SetCurrentProcessExplicitAppUserModelID(RAPTOR_APP_ID);
    if (FAILED(hr)) wprintf(L"[WARNING] Could not set AppUserModelID (0x%08lX).\n", (unsigned long)hr);

    wchar_t exe_path[32768];
    DWORD n = GetModuleFileNameW(NULL, exe_path, (DWORD)_countof(exe_path));
    if (n == 0 || n >= _countof(exe_path)) {
        log_error(L"Could not determine launcher path.");
        goto failed;
    }
    wchar_t* slash = wcsrchr(exe_path, L'\\');
    if (!slash) { log_error(L"Invalid launcher path."); goto failed; }
    slash[1] = L'\0';
    wprintf(L"[INFO] App directory: %ls\n", exe_path);

    wchar_t jar_path[32768];
    if (!join_path(jar_path, _countof(jar_path), exe_path, JAR_NAME) || !file_exists(jar_path)) {
        log_error(L"Raptor-Release.jar was not found beside the launcher.");
        goto failed;
    }
    wprintf(L"[OK] JAR: %ls\n", jar_path);

    wchar_t java_path[32768];
    if (!find_java(exe_path, java_path, _countof(java_path))) {
        log_error(L"Java was not found. Rebuild the portable package with the bundled runtime, or install a compatible Java runtime.");
        goto failed;
    }

    wchar_t command[32768];
    if (swprintf_s(command, _countof(command),
        L"\"%ls\" --enable-native-access=ALL-UNNAMED -jar \"%ls\"",
        java_path, jar_path) < 0) {
        log_error(L"Could not construct Java command.");
        goto failed;
    }
    wprintf(L"[INFO] Working directory: %ls\n[INFO] Starting Raptor...\n\n", exe_path);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessW(java_path, command, NULL, NULL, TRUE, 0, NULL, exe_path, &si, &pi)) {
        fwprintf(stderr, L"[ERROR] CreateProcessW failed: %lu\n", GetLastError());
        goto failed;
    }

    wprintf(L"[OK] Java started (PID %lu). Output follows.\n", pi.dwProcessId);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    wprintf(L"\n[RAPTOR] Java exit code: %lu\n", exit_code);
    wprintf(L"Press ENTER to close this terminal...");
    (void)getwchar();
    return (int)exit_code;

failed:
    wprintf(L"\nPress ENTER to close this terminal...");
    (void)getwchar();
    return 1;
}
