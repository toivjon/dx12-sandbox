// include windows headers without unnecessary APIs.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

// undefine min macro and use the std::max from the <algorithm>
#if defined(min)
#undef min
#endif

// undefine max macro and use the std::max from the <algorithm>
#if defined(max)
#undef max
#endif

// include the required DirectX headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// include support for I/O streams.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

// ============================================================================

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// ============================================================================

using namespace Microsoft::WRL;
using namespace std::chrono;

// ============================================================================

// the name of the window class required by the WINAPI.
static const auto CLASS_NAME = "DX12-SANDBOX-WC";

// the initial width of the window.
static const auto WIDTH = 800;
// the initial height of the window.
static const auto HEIGHT = 600;

// the amount of swap chain buffers.
static const auto BUFFER_COUNT = 2;

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
    case WM_KEYDOWN:
      switch (wParam) {
        case VK_ESCAPE:
          PostQuitMessage(0);
          break;
      }
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

ComPtr<ID3D12Device> createDXDevice(ComPtr<IDXGIAdapter4> adapter)
{
  // try to create a new DX12 device from the provided adapter.
  ComPtr<ID3D12Device2> device;
  auto result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
  if (FAILED(result)) {
    std::cout << "D3D12CreateDevice: " << result << std::endl;
    throw new std::runtime_error("Failed to create DX12 device");
  }

  #if defined(_DEBUG)
  // try to access the DX12 device info queue.
  ComPtr<ID3D12InfoQueue> infoQueue;
  if (SUCCEEDED(device.As(&infoQueue))) {
    // tell queue that we want to break when something goes wrong.
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    // create an array of severity levels to be ignored.
    std::vector<D3D12_MESSAGE_SEVERITY> severities = { D3D12_MESSAGE_SEVERITY_INFO };

    // create an array to ignore non-critical warnings.
    std::vector<D3D12_MESSAGE_ID> deniedIds = {
      D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
      D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
      D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
    };

    // construct a queue filter to ignore certain severity levels and warnings.
    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumSeverities = severities.size();
    filter.DenyList.pSeverityList = &severities[0];
    filter.DenyList.NumIDs = deniedIds.size();
    filter.DenyList.pIDList = &deniedIds[0];

    // try to activate the created queue filter.
    result = infoQueue->PushStorageFilter(&filter);
    if (FAILED(result)) {
      std::cout << "infoQueue->PushStorageFilter: " << result << std::endl;
      throw new std::runtime_error("Failed to activate DX12 info queue filter");
    }
  }
  #endif

  return device;
}

// ============================================================================

ComPtr<ID3D12CommandQueue> createDXCommandQueue(ComPtr<ID3D12Device> device)
{
  // create a descriptor for the command queue.
  D3D12_COMMAND_QUEUE_DESC descriptor = {};
  descriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  descriptor.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  descriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  descriptor.NodeMask = 0;

  // try to create a new command queue for the target device.
  ComPtr<ID3D12CommandQueue> commandQueue;
  auto result = device->CreateCommandQueue(&descriptor, IID_PPV_ARGS(&commandQueue));
  if (FAILED(result)) {
    std::cout << "device->CreateCommandQueue: " << result << std::endl;
    throw new std::runtime_error("Failed to create command queue");
  }

  return commandQueue;
}

// ============================================================================

ComPtr<IDXGISwapChain4> createDXGISwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue)
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

  // create a descriptor for the swap chain.
  DXGI_SWAP_CHAIN_DESC1 descriptor = {};
  descriptor.Width = WIDTH;
  descriptor.Height = HEIGHT;
  descriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  descriptor.Stereo = false;
  descriptor.SampleDesc = { 1, 0 };
  descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  descriptor.BufferCount = BUFFER_COUNT;
  descriptor.Scaling = DXGI_SCALING_STRETCH;
  descriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  descriptor.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  descriptor.Flags = 0;

  // try to create the swap chain.
  ComPtr<IDXGISwapChain1> swapChain;
  result = factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &descriptor, nullptr, nullptr, &swapChain);
  if (FAILED(result)) {
    std::cout << "factory->CreateSwapChainForHwnd: " << result << std::endl;
    throw new std::runtime_error("Failed to create DXGI swap chain");
  }

  // cast the created swap chain into correct version.
  ComPtr<IDXGISwapChain4> swapChain4;
  result = swapChain.As(&swapChain4);
  if (FAILED(result)) {
    std::cout << "swapChain.As: " << result << std::endl;
    throw new std::runtime_error("Failed to cast DXGISwapChain1 to DXGISwapChain4");
  }

  return swapChain4;
}

// ============================================================================

ComPtr<ID3D12DescriptorHeap> createDXDescriptorHeap(ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
  // create a descriptor for the descriptor heap.
  D3D12_DESCRIPTOR_HEAP_DESC descriptor = {};
  descriptor.NumDescriptors = BUFFER_COUNT;
  descriptor.Type = type;

  // try to create the descriptor heap.
  ComPtr<ID3D12DescriptorHeap> descriptorHeap;
  auto result = device->CreateDescriptorHeap(&descriptor, IID_PPV_ARGS(&descriptorHeap));
  if (FAILED(result)) {
    std::cout << "device->CreateDescriptorHeap: " << result << std::endl;
    throw new std::runtime_error("Failed to create descriptor heap");
  }

  return descriptorHeap;
}

// ============================================================================

ComPtr<ID3D12CommandAllocator> createDXCommandAllocator(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
{
  // try to create a new command allocator.
  ComPtr<ID3D12CommandAllocator> allocator;
  auto result = device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));
  if (FAILED(result)) {
    std::cout << "device->CreateCommandAllocator: " << result << std::endl;
    throw new std::runtime_error("Failed to create command allocator");
  }

  return allocator;
}

// ============================================================================

ComPtr<ID3D12GraphicsCommandList> createDXCommandList(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandAllocator> commandAllocator)
{
  // try to create a new command list.
  ComPtr<ID3D12GraphicsCommandList> commandList;
  auto result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
  if (FAILED(result)) {
    std::cout << "device->CreateCommandList: " << result << std::endl;
    throw new std::runtime_error("Failed to create command list");
  }

  return commandList;
}

// ============================================================================

ComPtr<ID3D12Fence> createDXFence(ComPtr<ID3D12Device> device)
{
  // try to create a new fence for the target device.
  ComPtr<ID3D12Fence> fence;
  auto result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
  if (FAILED(result)) {
    std::cout << "device->CreateFence: " << result << std::endl;
    throw new std::runtime_error("Failed to create a new fence");
  }

  return fence;
}

// ============================================================================

HANDLE createEvent()
{
  auto event = CreateEvent(nullptr, false, false, nullptr);
  if (event == nullptr) {
    std::cout << "CreateEvent failed" << std::endl;
    throw new std::runtime_error("Failed to create new event");
  }

  return event;
}

// ============================================================================

void waitFence(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE event, milliseconds duration)
{
  if (fence->GetCompletedValue() < fenceValue)
  {
    // specify which event to trigger after fence has been finished.
    auto result = fence->SetEventOnCompletion(fenceValue, event);
    if (FAILED(result)) {
      std::cout << "fence->SetEventOnCompletion: " << result << std::endl;
      throw new std::runtime_error("Failed to set event for fence completion");
    }

    // wait for a signal or until the given duration has elapsed.
    WaitForSingleObject(event, static_cast<DWORD>(duration.count()));
  }
}

// ============================================================================

uint64_t signalFence(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& value)
{
  // increment the fence value to indicate a new signal.
  uint64_t signalValue = ++value;

  // try to signal the fence with the incremented signal value.
  auto result = commandQueue->Signal(fence.Get(), signalValue);
  if (FAILED(result)) {
    std::cout << "commandQueue->Signal: " << result << std::endl;
    throw new std::runtime_error("Failed to signal fence");
  }

  return signalValue;
}

// ============================================================================

void flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& value, HANDLE event)
{
  auto signalValue = signalFence(commandQueue, fence, value);
  waitFence(fence, signalValue, event, milliseconds::max());
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
  auto device = createDXDevice(adapter);
  auto commandQueue = createDXCommandQueue(device);
  auto swapChain = createDXGISwapChain(hwnd, commandQueue);
  auto descriptorHeap = createDXDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  auto commandAllocator = createDXCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
  auto commandList = createDXCommandList(device, commandAllocator);
  auto fence = createDXFence(device);
  auto fenceEvent = createEvent();
  uint64_t fenceValue = 0u;

  // set the window visible.
  ShowWindow(hwnd, SW_SHOW);
  
  // operate WINAPI cycle which runs until an exit message is received.
  MSG msg = {};
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  flush(commandQueue, fence, fenceValue, fenceEvent);

  CloseHandle(fenceEvent);
  destroyWindow(hwnd);
  unregisterWindowClass();
  return 0;
}