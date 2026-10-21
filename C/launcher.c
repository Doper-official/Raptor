
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shobjidl.h>
#include <wchar.h>

#define JAVA_PATH \
    L"C:\\Program Files\\Eclipse Adoptium\\jdk-27.0.0.35-hotspot\\bin\\javaw.exe"

#define JAR_NAME L"Raptor-Test.jar"
#define RAPTOR_APP_ID L"Doper.Raptor"

static void show_error(const wchar_t* message)
{
    MessageBoxW(
        NULL,
        message,
        L"Raptor Launcher",
        MB_OK | MB_ICONERROR
    );
}

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE previous_instance,
    PWSTR command_line,
    int show_command
)
{
    (void)instance;
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    /*
     * Give the launcher a stable Windows taskbar identity.
     */
    HRESULT app_id_result =
        SetCurrentProcessExplicitAppUserModelID(RAPTOR_APP_ID);

    if (FAILED(app_id_result)) {
        show_error(L"Could not set the launcher taskbar identity.");
        return 1;
    }

    wchar_t exe_path[MAX_PATH];
    wchar_t jar_path[MAX_PATH];
    wchar_t command[MAX_PATH * 3];

    DWORD length = GetModuleFileNameW(
        NULL,
        exe_path,
        MAX_PATH
    );

    if (length == 0 || length >= MAX_PATH) {
        show_error(L"Could not determine the launcher's location.");
        return 1;
    }

    wchar_t* separator = wcsrchr(exe_path, L'\\');

    if (separator == NULL) {
        show_error(L"Invalid launcher path.");
        return 1;
    }

    *(separator + 1) = L'\0';

    if (swprintf_s(
        jar_path,
        MAX_PATH,
        L"%s%s",
        exe_path,
        JAR_NAME
    ) < 0) {
        show_error(L"Could not construct the JAR path.");
        return 1;
    }

    if (GetFileAttributesW(JAVA_PATH) == INVALID_FILE_ATTRIBUTES) {
        show_error(
            L"Java was not found at the configured path.\n\n"
            L"Check the JDK 27 installation path in launcher.c."
        );
        return 1;
    }

    if (GetFileAttributesW(jar_path) == INVALID_FILE_ATTRIBUTES) {
        show_error(
            L"Raptor-Test.jar was not found beside "
            L"RaptorJarLauncher.exe."
        );
        return 1;
    }

    if (swprintf_s(
        command,
        MAX_PATH * 3,
        L"\"%s\" --enable-native-access=ALL-UNNAMED -jar \"%s\"",
        JAVA_PATH,
        jar_path
    ) < 0) {
        show_error(L"Could not construct the Java command.");
        return 1;
    }

    STARTUPINFOW startup = { 0 };
    startup.cb = sizeof(startup);

    PROCESS_INFORMATION process = { 0 };

    /*
     * Use the launcher's directory as the working directory.
     */
    BOOL launched = CreateProcessW(
        JAVA_PATH,
        command,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        exe_path,
        &startup,
        &process
    );

    if (!launched) {
        wchar_t error[256];

        swprintf_s(
            error,
            256,
            L"Failed to start Java.\nWindows error: %lu",
            GetLastError()
        );

        show_error(error);
        return 1;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    return 0;
}