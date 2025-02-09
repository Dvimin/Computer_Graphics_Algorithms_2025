#define UNICODE  // Используем Unicode-версию WinAPI
#include <windows.h>

// Обработчик сообщений окна
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Регистрация класса окна
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DirectXWindowClass";  // Unicode строка

    RegisterClass(&wc);

    // Создание окна (теперь используется CreateWindowExW)
    HWND hWnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"DirectX 11 Window",  // Unicode строка
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return -1;

    ShowWindow(hWnd, nCmdShow);

    // Цикл обработки сообщений
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
