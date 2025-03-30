#include "main.h"
#include "d3d_utils.h"

bool isMouseCaptured = false;
int lastMouseX = 0, lastMouseY = 0;

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DirectXWindowClass";
    RegisterClass(&wc);

    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowEx(0, wc.lpszClassName, L"DirectX 11 Window",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    InitD3D(hWnd);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            RenderFrame();
        }
    }

    CleanupD3D();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        ResizeD3D(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:
            cameraPitch += 0.05f;
            cameraUpdated = true;
            break;
        case VK_DOWN:
            cameraPitch -= 0.05f;
            cameraUpdated = true;
            break;
        }
        return 0;

    case WM_LBUTTONDOWN:
    isMouseCaptured = true;
    lastMouseX = GET_X_LPARAM(lParam);
    lastMouseY = GET_Y_LPARAM(lParam);
    SetCapture(hWnd);
    ShowCursor(FALSE); 
    return 0;

case WM_LBUTTONUP:
    isMouseCaptured = false;
    ReleaseCapture();
    ShowCursor(TRUE); 
    return 0;

case WM_MOUSEMOVE:
    if (isMouseCaptured) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        OnMouseMove(x, y, true);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}