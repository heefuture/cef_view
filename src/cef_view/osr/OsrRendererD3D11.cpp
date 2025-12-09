/**
 * @file OsrRendererD3D11.cpp
 * @brief Direct3D 11 hardware-accelerated off-screen renderer implementation
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#include "OsrRendererD3D11.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <algorithm>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace cefview {

// Helper macro: check HRESULT
#define HR_CHECK(exp)  \
    if (FAILED(exp)) { \
        return false;  \
    }

// Vertex structure
struct Vertex {
    XMFLOAT3 position;  ///< Position (NDC coordinates)
    XMFLOAT2 texcoord;  ///< Texture coordinates
};

// Vertex shader source code
static const char* g_vertexShaderSource = R"(
struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = input.pos;
	output.tex = input.tex;
	return output;
}
)";

// Pixel shader source code
static const char* g_pixelShaderSource = R"(
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target
{
	return tex0.Sample(samp0, input.tex);
}
)";

OsrRendererD3D11::OsrRendererD3D11(HWND hwnd, int width, int height)
    : _hwnd(hwnd)
    , _width(width)
    , _height(height)
    , _showPopup(false) {
    _popupRect.x = 0;
    _popupRect.y = 0;
    _popupRect.width = 0;
    _popupRect.height = 0;
}

OsrRendererD3D11::~OsrRendererD3D11() {
    uninitialize();
}

bool OsrRendererD3D11::initialize() {
    // Create device and swap chain
    if (!createDeviceAndSwapchain()) {
        return false;
    }

    // Create shader resources
    if (!createShaderResource()) {
        uninitialize();
        return false;
    }

    // Create sampler
    if (!createSampler()) {
        uninitialize();
        return false;
    }

    // Create blender
    if (!createBlender()) {
        uninitialize();
        return false;
    }

    // Create render target view
    if (!createRenderTargetView()) {
        uninitialize();
        return false;
    }

    // Setup rendering pipeline
    setupPipeline();

    return true;
}

void OsrRendererD3D11::uninitialize() {
    // ComPtr will automatically release resources, explicitly reset to ensure proper order
    _cefViewVertexBuffer.Reset();
    _cefPopupVertexBuffer.Reset();
    _cefViewShaderResourceView.Reset();
    _cefPopupShaderResourceView.Reset();
    _cefViewTexture.Reset();
    _cefPopupTexture.Reset();
    _renderTargetView.Reset();
    _blenderState.Reset();
    _samplerState.Reset();
    _pixelShader.Reset();
    _vertexShader.Reset();
    _inputLayout.Reset();
    _swapChain.Reset();
    _d3dContext.Reset();
    _d3dDevice.Reset();
}

bool OsrRendererD3D11::createDeviceAndSwapchain() {
    // Create D3D11 device and context
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);
    D3D_FEATURE_LEVEL featureLevel;

    HR_CHECK(D3D11CreateDevice(nullptr,                      // Default adapter
                                D3D_DRIVER_TYPE_HARDWARE,    // Hardware device
                                nullptr,                     // No software rasterizer
                                createDeviceFlags,           // Device flags
                                featureLevels,               // Feature level array
                                numFeatureLevels,            // Array size
                                D3D11_SDK_VERSION,           // SDK version
                                _d3dDevice.GetAddressOf(),   // Output device
                                &featureLevel,               // Actual feature level
                                _d3dContext.GetAddressOf()   // Output context
                                ));

    // Get DXGI device
    ComPtr<IDXGIDevice> dxgiDevice;
    HR_CHECK(_d3dDevice.As(&dxgiDevice));

    // Get DXGI adapter
    ComPtr<IDXGIAdapter> dxgiAdapter;
    HR_CHECK(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

    // Get DXGI factory
    ComPtr<IDXGIFactory> dxgiFactory;
    HR_CHECK(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

    // Create swap chain description
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;                                     // Double buffering
    swapChainDesc.BufferDesc.Width = _width;
    swapChainDesc.BufferDesc.Height = _height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;      // BGRA format
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = _hwnd;
    swapChainDesc.SampleDesc.Count = 1;                                // No multisampling
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;                                     // Windowed mode
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;          // Flip mode
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Create swap chain
    HR_CHECK(dxgiFactory->CreateSwapChain(_d3dDevice.Get(), &swapChainDesc, _swapChain.GetAddressOf()));

    // Disable Alt+Enter fullscreen toggle
    HR_CHECK(dxgiFactory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER));

    return true;
}

bool OsrRendererD3D11::createShaderResource() {
    // Compile vertex shader
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(g_vertexShaderSource,
                            strlen(g_vertexShaderSource),
                            nullptr,
                            nullptr,
                            nullptr,
                            "main",
                            "vs_5_0",
                            0,
                            0,
                            vsBlob.GetAddressOf(),
                            errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    // Create vertex shader
    HR_CHECK(_d3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(),
                                            vsBlob->GetBufferSize(),
                                            nullptr,
                                            _vertexShader.GetAddressOf()));

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    UINT numElements = ARRAYSIZE(layout);

    HR_CHECK(_d3dDevice->CreateInputLayout(layout,
                                           numElements,
                                           vsBlob->GetBufferPointer(),
                                           vsBlob->GetBufferSize(),
                                           _inputLayout.GetAddressOf()));

    // Compile pixel shader
    ComPtr<ID3DBlob> psBlob;
    hr = D3DCompile(g_pixelShaderSource,
                    strlen(g_pixelShaderSource),
                    nullptr,
                    nullptr,
                    nullptr,
                    "main",
                    "ps_5_0",
                    0,
                    0,
                    psBlob.GetAddressOf(),
                    errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    // Create pixel shader
    HR_CHECK(_d3dDevice->CreatePixelShader(psBlob->GetBufferPointer(),
                                           psBlob->GetBufferSize(),
                                           nullptr,
                                           _pixelShader.GetAddressOf()));

    // Create View texture (initial size, will be dynamically updated later)
    if (_width > 0 && _height > 0) {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = _width;
        texDesc.Height = _height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        HR_CHECK(_d3dDevice->CreateTexture2D(&texDesc, nullptr, _cefViewTexture.GetAddressOf()));

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        HR_CHECK(_d3dDevice->CreateShaderResourceView(_cefViewTexture.Get(),
                                                       &srvDesc,
                                                       _cefViewShaderResourceView.GetAddressOf()));
    }

    return true;
}

bool OsrRendererD3D11::createRenderTargetView() {
    // Get back buffer from swap chain
    ComPtr<ID3D11Texture2D> backBuffer;
    HR_CHECK(_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create render target view
    HR_CHECK(_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, _renderTargetView.GetAddressOf()));

    return true;
}

bool OsrRendererD3D11::createSampler() {
    // Create sampler state (linear filtering)
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HR_CHECK(_d3dDevice->CreateSamplerState(&samplerDesc, _samplerState.GetAddressOf()));

    return true;
}

bool OsrRendererD3D11::createBlender() {
    // Create blend state (alpha blending)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HR_CHECK(_d3dDevice->CreateBlendState(&blendDesc, _blenderState.GetAddressOf()));

    return true;
}

void OsrRendererD3D11::setupPipeline() {
    // Set input layout
    _d3dContext->IASetInputLayout(_inputLayout.Get());

    // Set primitive topology (triangle list)
    _d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set vertex shader
    _d3dContext->VSSetShader(_vertexShader.Get(), nullptr, 0);

    // Set pixel shader
    _d3dContext->PSSetShader(_pixelShader.Get(), nullptr, 0);

    // Set sampler
    _d3dContext->PSSetSamplers(0, 1, _samplerState.GetAddressOf());

    // Set blend state
    float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    _d3dContext->OMSetBlendState(_blenderState.Get(), blendFactor, 0xffffffff);

    // Set render target
    _d3dContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), nullptr);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(_width);
    viewport.Height = static_cast<float>(_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _d3dContext->RSSetViewports(1, &viewport);
}

void OsrRendererD3D11::updateFrameData(CefRenderHandler::PaintElementType type,
                                       const CefRenderHandler::RectList& dirtyRects,
                                       const void* buffer,
                                       int width,
                                       int height) {
    if (!_d3dDevice || !_d3dContext) {
        return;
    }

    // Determine if this is View or Popup
    bool isView = (type == PET_VIEW);
    ComPtr<ID3D11Texture2D>& texture = isView ? _cefViewTexture : _cefPopupTexture;
    ComPtr<ID3D11ShaderResourceView>& srv = isView ? _cefViewShaderResourceView : _cefPopupShaderResourceView;

    // Check if texture size needs to be recreated
    if (texture) {
        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        if (desc.Width != static_cast<UINT>(width) || desc.Height != static_cast<UINT>(height)) {
            // Size changed, recreate texture
            texture.Reset();
            srv.Reset();
        }
    }

    // If texture doesn't exist, create new texture
    if (!texture) {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // CEF uses BGRA format
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        if (FAILED(_d3dDevice->CreateTexture2D(&texDesc, nullptr, texture.GetAddressOf()))) {
            return;
        }

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        if (FAILED(_d3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, srv.GetAddressOf()))) {
            texture.Reset();
            return;
        }
    }

    // Update texture data (only update dirty rectangles)
    for (size_t i = 0; i < dirtyRects.size(); ++i) {
        const CefRect& rect = dirtyRects[i];

        // Calculate source data offset
        int srcPitch = width * 4;  // 4 bytes per pixel (BGRA)
        const unsigned char* srcData = static_cast<const unsigned char*>(buffer);
        srcData += rect.y * srcPitch + rect.x * 4;

        // Update subresource
        D3D11_BOX destBox = {};
        destBox.left = rect.x;
        destBox.top = rect.y;
        destBox.right = rect.x + rect.width;
        destBox.bottom = rect.y + rect.height;
        destBox.front = 0;
        destBox.back = 1;

        _d3dContext->UpdateSubresource(texture.Get(), 0, &destBox, srcData, srcPitch, 0);
    }
}

void OsrRendererD3D11::updateSharedTexture(CefRenderHandler::PaintElementType type,
                                           const CefAcceleratedPaintInfo& info) {
    if (!_d3dDevice || !_d3dContext) {
        return;
    }

    // Determine if this is the View or Popup layer
    bool isView = (type == PET_VIEW);
    ComPtr<ID3D11Texture2D>& texture = isView ? _cefViewTexture : _cefPopupTexture;
    ComPtr<ID3D11ShaderResourceView>& srv = isView ? _cefViewShaderResourceView : _cefPopupShaderResourceView;

    // Extract shared texture handle from CEF
    void* sharedHandle = info.shared_texture_handle;
    if (!sharedHandle) {
        return;
    }

    // Query for ID3D11Device1 interface (required for OpenSharedResource1)
    ComPtr<ID3D11Device1> device1;
    HRESULT hr = _d3dDevice.As(&device1);
    if (FAILED(hr)) {
        return;
    }

    // Open the shared texture resource using OpenSharedResource1
    ComPtr<ID3D11Texture2D> sharedTexture;
    hr = device1->OpenSharedResource1(sharedHandle, IID_PPV_ARGS(&sharedTexture));
    if (FAILED(hr)) {
        return;
    }

    // Check if local texture needs to be recreated
    bool needUpdate = false;
    if (!texture) {
        needUpdate = true;
    } else {
        // Compare dimensions
        D3D11_TEXTURE2D_DESC existingDesc, newDesc;
        texture->GetDesc(&existingDesc);
        sharedTexture->GetDesc(&newDesc);
        if (existingDesc.Width != newDesc.Width || existingDesc.Height != newDesc.Height) {
            needUpdate = true;
        }
    }

    if (needUpdate) {
        // Get shared texture description
        D3D11_TEXTURE2D_DESC sharedDesc;
        sharedTexture->GetDesc(&sharedDesc);

        // Create local texture matching the shared texture format
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = sharedDesc.Width;
        texDesc.Height = sharedDesc.Height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = sharedDesc.Format;  // Match shared texture format
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        texture.Reset();
        srv.Reset();

        if (FAILED(_d3dDevice->CreateTexture2D(&texDesc, nullptr, texture.GetAddressOf()))) {
            return;
        }

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        if (FAILED(_d3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, srv.GetAddressOf()))) {
            texture.Reset();
            return;
        }
    }

    // Copy shared texture to local texture (GPU-to-GPU copy, zero CPU overhead)
    _d3dContext->CopyResource(texture.Get(), sharedTexture.Get());
}

void OsrRendererD3D11::render() {
    if (!_d3dContext || !_renderTargetView) {
        return;
    }

    // Clear render target (black background)
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    _d3dContext->ClearRenderTargetView(_renderTargetView.Get(), clearColor);

    // Draw View texture (fullscreen quad)
    if (_cefViewTexture && _cefViewShaderResourceView) {
        // Create or update vertex buffer
        if (!_cefViewVertexBuffer) {
            createQuadVertexBuffer(-1.0f, 1.0f, 2.0f, -2.0f, _width, _height, _cefViewVertexBuffer.GetAddressOf());
        }

        if (_cefViewVertexBuffer) {
            // Set vertex buffer
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            _d3dContext->IASetVertexBuffers(0, 1, _cefViewVertexBuffer.GetAddressOf(), &stride, &offset);

            // Set texture
            _d3dContext->PSSetShaderResources(0, 1, _cefViewShaderResourceView.GetAddressOf());

            // Draw 6 vertices (2 triangles)
            _d3dContext->Draw(6, 0);
        }
    }

    // Draw Popup texture (if visible)
    if (_showPopup && _cefPopupTexture && _cefPopupShaderResourceView) {
        // Calculate Popup position in NDC coordinate system
        float x = (float)_popupRect.x / _width * 2.0f - 1.0f;
        float y = 1.0f - (float)_popupRect.y / _height * 2.0f;
        float w = (float)_popupRect.width / _width * 2.0f;
        float h = (float)_popupRect.height / _height * 2.0f;

        // Create or update Popup vertex buffer
        if (!_cefPopupVertexBuffer) {
            createQuadVertexBuffer(x, y, w, -h, _width, _height, _cefPopupVertexBuffer.GetAddressOf());
        }

        if (_cefPopupVertexBuffer) {
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            _d3dContext->IASetVertexBuffers(0, 1, _cefPopupVertexBuffer.GetAddressOf(), &stride, &offset);
            _d3dContext->PSSetShaderResources(0, 1, _cefPopupShaderResourceView.GetAddressOf());
            _d3dContext->Draw(6, 0);
        }
    }

    // Present to screen
    _swapChain->Present(1, 0);  // VSync
}

void OsrRendererD3D11::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    _width = width;
    _height = height;

    if (!_swapChain) {
        return;
    }

    // Release render target view (must be released before resizing swap chain)
    _renderTargetView.Reset();
    _d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    _d3dContext->Flush();

    // Resize swap chain buffers
    HRESULT hr = _swapChain->ResizeBuffers(0,                             // Keep buffer count
                                           width,
                                           height,
                                           DXGI_FORMAT_UNKNOWN,           // Keep format
                                           DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    if (FAILED(hr)) {
        // Device may be lost, attempt recovery
        handleDeviceLost();
        return;
    }

    // Recreate render target view
    createRenderTargetView();

    // Reset pipeline
    setupPipeline();

    // Recreate vertex buffers (size changed)
    _cefViewVertexBuffer.Reset();
    _cefPopupVertexBuffer.Reset();
}

void OsrRendererD3D11::handleDeviceLost() {
    // Release all resources
    uninitialize();

    // Reinitialize
    initialize();
}

void OsrRendererD3D11::updatePopupVisibility(bool visible) {
    _showPopup = visible;

    if (!visible) {
        // Release Popup resources when hidden
        _cefPopupTexture.Reset();
        _cefPopupShaderResourceView.Reset();
        _cefPopupVertexBuffer.Reset();
    }
}

void OsrRendererD3D11::updatePopupRect(const CefRect& rect) {
    _popupRect = rect;

    // Recreate Popup vertex buffer (position changed)
    _cefPopupVertexBuffer.Reset();
}

bool OsrRendererD3D11::createQuadVertexBuffer(float x,
                                              float y,
                                              float w,
                                              float h,
                                              int viewWidth,
                                              int viewHeight,
                                              ID3D11Buffer** ppBuffer) {
    // Define 6 vertices (2 triangles forming a rectangle)
    // Vertex order: top-left, top-right, bottom-left, bottom-left, top-right, bottom-right
    Vertex vertices[6] = {
        {XMFLOAT3(x, y, 0.0f), XMFLOAT2(0.0f, 0.0f)},          // Top-left
        {XMFLOAT3(x + w, y, 0.0f), XMFLOAT2(1.0f, 0.0f)},      // Top-right
        {XMFLOAT3(x, y + h, 0.0f), XMFLOAT2(0.0f, 1.0f)},      // Bottom-left
        {XMFLOAT3(x, y + h, 0.0f), XMFLOAT2(0.0f, 1.0f)},      // Bottom-left
        {XMFLOAT3(x + w, y, 0.0f), XMFLOAT2(1.0f, 0.0f)},      // Top-right
        {XMFLOAT3(x + w, y + h, 0.0f), XMFLOAT2(1.0f, 1.0f)},  // Bottom-right
    };

    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HR_CHECK(_d3dDevice->CreateBuffer(&bufferDesc, &initData, ppBuffer));

    return true;
}

}  // namespace cefview
