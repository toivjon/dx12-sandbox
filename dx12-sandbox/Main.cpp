// include windows headers without unnecessary APIs.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// include the main DirectX header.
#include <d3d12.h>

// include support for I/O streams.
#include <iostream>

// ============================================================================

// the name of the window class required by the WINAPI.
static const auto CLASS_NAME = "DX12-SANDBOX-WC";

// the initial width of the window.
static const auto WIDTH = 800;
// the initial height of the window.
static const auto HEIGHT = 600;

// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_CLOSE:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================================================

void registerWindowClass()
{
  // construct a new empty window class descriptor.
  WNDCLASSEX windowClass = {};

  // specify the desired window class definitions.
  windowClass.cbClsExtra = 0;
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.cbWndExtra = 0;
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  windowClass.hIconSm = LoadIcon(GetModuleHandle(nullptr), nullptr);
  windowClass.hInstance = GetModuleHandle(nullptr);
  windowClass.lpfnWndProc = WndProc;
  windowClass.lpszMenuName = nullptr;
  windowClass.lpszClassName = CLASS_NAME;
  windowClass.style = CS_HREDRAW | CS_VREDRAW;

  // try to register the window class and check results.
  if (RegisterClassEx(&windowClass) == 0) {
    std::cout << "RegisterClassEx: " << GetLastError() << std::endl;
    throw new std::runtime_error("Window class registration failed");
  }
}

// ============================================================================

void unregisterWindowClass()
{
  // try to unregister the window class and check results.
  if (UnregisterClass(CLASS_NAME, GetModuleHandle(nullptr)) == 0) {
    std::cout << "UnregisterClass: " << GetLastError() << std::endl;
    throw new std::runtime_error("Window class unregistration failed");
  }
}

// ============================================================================

HWND createWindow()
{
  // construct a new window with the desired definitions.
  HWND hwnd = CreateWindowEx(
    WS_EX_CLIENTEDGE,
    CLASS_NAME,
    "DirectX 12 - Sandbox",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    WIDTH,
    HEIGHT,
    nullptr,
    nullptr,
    GetModuleHandle(nullptr),
    nullptr);

  // check how the operation succeeded.
  if (hwnd == nullptr) {
    std::cout << "CreateWindowEx: " << GetLastError() << std::endl;
    throw new std::runtime_error("Window creation failed");
  }

  // operation succeeded...
  return hwnd;
}

// ============================================================================

void destroyWindow(HWND hwnd)
{
  // try to destroy the given window and check results.
  if (DestroyWindow(hwnd) == 0) {
    std::cout << "DestroyWindow: " << GetLastError() << std::endl;
    throw new std::runtime_error("Window destruction failed");
  }
}

// ============================================================================

int main()
{
  registerWindowClass();
  auto hwnd = createWindow();
  
  // TODO ...

  destroyWindow(hwnd);
  unregisterWindowClass();
  return 0;
}