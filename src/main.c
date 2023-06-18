#define UNICODE

#include <Windows.h>
#include <WinUser.h>
#include <libloaderapi.h>
#include <minwindef.h>
#include <wchar.h>
#include <errhandlingapi.h>
#include <winbase.h>
#include <winnt.h>
#include <strsafe.h>
#include <basetsd.h>
#include <minwinbase.h>
#include <processthreadsapi.h>
#include <stdio.h>

typedef struct state {
    BOOL active;
} state_t;

void displayError(LPWSTR err) {
    MessageBox(NULL, err, TEXT("Error"), MB_OK | MB_ICONERROR);
}
 
LPWSTR getErrorMessage(DWORD code) {
    LPVOID buf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &buf,
        0,
        NULL
    );
    return buf;
}

LPWSTR formatError(DWORD code, LPWSTR msg, LPWSTR function) {
    SIZE_T desiredSize = (lstrlen(msg) + lstrlen(function) + 40) * sizeof(WCHAR);
    LPWSTR buf = LocalAlloc(LMEM_ZEROINIT, desiredSize);
    StringCchPrintf(
        buf,
        LocalSize(buf) / sizeof(WCHAR),
        TEXT("[%d] %s: %s"),
        code, function, msg
    );
    return buf;
}

void error(LPWSTR function, BOOL fatal) {
    DWORD code = GetLastError();
    LPWSTR errorMessage = getErrorMessage(code);
    LPWSTR formattedError = formatError(code, errorMessage, function);

    displayError(formattedError);

    LocalFree(formattedError);
    LocalFree(errorMessage);

    if (fatal) {
        ExitProcess(code);
    }
}

LRESULT onHotkey(HWND hwnd) {
    state_t *state = (state_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    state->active = !state->active;
    LONG cur_style = GetWindowLong(hwnd, GWL_EXSTYLE);
    DWORD style = 0;

    if (state->active) {
        style = cur_style | WS_EX_TRANSPARENT | WS_EX_LAYERED;
    } else {
        style = cur_style & !(WS_EX_TRANSPARENT | WS_EX_LAYERED);
    }

    SetWindowLong(hwnd, GWL_EXSTYLE, style);

    return 0;
}

LRESULT onCreate(HWND hwnd, LPARAM lParam) {
    CREATESTRUCT *create = (CREATESTRUCT*)lParam;
    state_t *state = (state_t*)create->lpCreateParams;            
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
    return 0;
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    state_t *state;
    switch (uMsg) {
        case WM_CREATE:
            return onCreate(hwnd, lParam);
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_HOTKEY:
            return onHotkey(hwnd);
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

HWND createWindow(const wchar_t *title, state_t *state) {
    const HMODULE hInstance = GetModuleHandle(NULL);

    WNDCLASS window_class = { };
    window_class.lpfnWndProc = windowProc;
    window_class.hInstance = hInstance;
    window_class.lpszClassName = title;

    if (!RegisterClass(&window_class)) {
        error(TEXT("RegisterClass"), TRUE);
    }

    const DWORD style = WS_SYSMENU | WS_EX_LAYERED;

    HWND hwnd = CreateWindowEx(
        0,
        title,
        title,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        (LPVOID) state
    );

    if (hwnd == NULL) {
        error(TEXT("CreateWindowEx"), TRUE);
    }

    return hwnd;
}

int main(int argc, char **argv) {
    state_t s = { };
    s.active = TRUE;

    HWND hwnd = createWindow(L"My window", &s);

    RegisterHotKey(hwnd, 1, 0x4000, 0x42); // b
    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        //SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 600, 400, 0); // here?
    }

    UnregisterHotKey(hwnd, 1);
}