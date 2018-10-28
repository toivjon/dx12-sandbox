# dx12-sandbox
A sandbox to test and play around with DirectX 12 API.

## Prerequisities
This sandbox requires that the host machine has a DirectX 12 supported GPU and Windows 10 installed.

## Notes About Using Direct3D 12
Procedure of initializing the Direct3D 12 goes as following.

(N is the amount of buffers used within the application)

1. [Optional] Enable debug layer.
2. [Optional] Enumerate and select a graphics adapter (IDXGIAdapter).
3. Create a device (ID3D12Device).
4. Create a command queue (ID3D12CommandQueue).
5. Create a swap chain (IDXGISwapChain).
6. Create a descriptor heap for render target views (ID3D12DescriptorHeap).
7. Create N-amount of render target views (ID3D12Resource).
8. Create a command allocator (ID3D12CommandAllocator).

Note that before initializing Direct3D, we also need to register window class and create a window.

Procedure of initializing resources (e.g. assets) for Direct3D 12 goes as following.

1. Serialize and create a root signature (ID3D12RootSignature).
2. Load and compile shaders (ID3DBlob).
4. Create a pipeline state object (ID3D12PipelineState).
5. Create and close a command list (ID3D12GraphicsCommandList).
6. Create and fill vertex buffer (ID3D12Resource).
7. Create a vertex buffer view (D3D12_VERTEX_BUFFER_VIEW).
8. Wait until resources are in sync with GPU (ID3D12Fence).

Procedure of rendering in Direct3D 12 goes as following.

1. Reset command list allocator for the next buffer (ID3D12CommandAllocator).
2. Reset command list for the next buffer (ID3D12GraphicsCommandList).
3. Set the graphics root signature.
4. Set viewport.
5. Set scissor rectangles.
6. Use barrier to indicate that backbuffer is now the render target.
7. Add commands into the command list.
8. Use barrier to indicate that backbuffer is being presented after commands have finished.
9. Close command list.
10. Execute command list.
11. Present the backbuffer.
12. Wait until GPU has finished.