// include windows headers without unnecessary APIs.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

// include the required DirectX headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// include support for I/O streams.
#include <iostream>

// ============================================================================

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// ============================================================================

using namespace Microsoft::WRL;

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

void enableDXDebugging()
{
  // try to catch a reference to the DX12 debug layer.
  ComPtr<ID3D12Debug> debug;
  auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
  if (FAILED(result)) {
    std::cout << "D3D12GetDebugInterface: " << result << std::endl;
    throw new std::runtime_error("Failed to access DX12 debug layer");
  }

  // enable the debugging layer.
  debug->EnableDebugLayer();
}

// ============================================================================

ComPtr<IDXGIAdapter4> selectDXGIAdapter()
{
  // specify debug flag when building in a debug mode.
  auto flags = 0u;
  #if defined(_DEBUG)
  flags = DXGI_CREATE_FACTORY_DEBUG;
  #endif

  // try to create a factory for DXGI instances.
  ComPtr<IDXGIFactory4> factory;
  auto result = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
  if (FAILED(result)) {
    std::cout << "CreateDXGIFactory2: " << result << std::endl;
    throw new std::runtime_error("DXGI factory creation failed");
  }

  // enumerate adapters and find the one with most dedicated video memory.
  auto maxVideoMemory = 0u;
  ComPtr<IDXGIAdapter1> adapter1;
  ComPtr<IDXGIAdapter4> adapter4;
  for (auto i = 0u; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
    // get the adapter descriptor info item.
    DXGI_ADAPTER_DESC1 descriptor;
    adapter1->GetDesc1(&descriptor);

    // skip software emulation based adapters.
    if ((descriptor.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
      continue;

    // skip adapters that cannot be created.
    if (FAILED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
      continue;

    // skip adapters which have less memory than the current maximum.
    if (descriptor.DedicatedVideoMemory < maxVideoMemory)
      continue;

    // we found a good candidate as the selected adapter.
    result = adapter1.As(&adapter4);
    if (FAILED(result)) {
      std::cout << "IDXGIAdapter1.As: " << result << std::endl;
      throw new std::runtime_error("Failed to cast DXGIAdapter1 to DXGIAdapter4");
    }
  }

  // return the result.
  return adapter4;
}

// ============================================================================

int main()
{
  #if defined(_DEBUG)
  enableDXDebugging();
  #endif

  registerWindowClass();
  auto hwnd = createWindow();
  auto adapter = selectDXGIAdapter();
  
  // TODO ...

  destroyWindow(hwnd);
  unregisterWindowClass();
  return 0;
}