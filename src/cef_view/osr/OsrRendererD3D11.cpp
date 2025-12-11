/**
 * @file OsrRendererD3D11.cpp
 * @brief Direct3D 11 hardware-accelerated off-screen renderer implementation
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#include "OsrRendererD3D11.h"

#if defined(_WIN32)

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <algorithm>

#include <utils/LogUtil.h>

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

OsrRendererD3D11::OsrRendererD3D11(HWND hwnd, int width, int height, bool transparent)
    : OsrRenderer(transparent)
    , _hwnd(hwnd) {
    _viewWidth = width;
    _viewHeight = height;
}

OsrRendererD3D11::~OsrRendererD3D11() {
    uninitialize();
}

bool OsrRendererD3D11::initialize() {
    LOGD << "initialize() called";

    if (!createDeviceAndSwapchain()) {
        LOGE << "createDeviceAndSwapchain() FAILED";
        return false;
    }
    LOGD << "createDeviceAndSwapchain() OK";

    if (!createShaderResource()) {
        LOGE << "createShaderResource() FAILED";
        uninitialize();
        return false;
    }
    LOGD << "createShaderResource() OK";

    if (!createSampler()) {
        LOGE << "createSampler() FAILED";
        uninitialize();
        return false;
    }
    LOGD << "createSampler() OK";

    if (!createBlender()) {
        LOGE << "createBlender() FAILED";
        uninitialize();
        return false;
    }
    LOGD << "createBlender() OK";

    if (!createRenderTargetView()) {
        LOGE << "createRenderTargetView() FAILED";
        uninitialize();
        return false;
    }
    LOGD << "createRenderTargetView() OK";

    setupPipeline();
    LOGI << "OsrRendererD3D11 initialized successfully";

    return true;
}

void OsrRendererD3D11::uninitialize() {
    _sharedTextureSRV.Reset();
    _sharedTexture.Reset();
    _sharedTextureHandle = nullptr;
    _cefViewVertexBuffer.Reset();
    _cefViewShaderResourceView.Reset();
    _cefViewTexture.Reset();
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

    HR_CHECK(D3D11CreateDevice(nullptr,
                                D3D_DRIVER_TYPE_HARDWARE,
                                nullptr,
                                createDeviceFlags,
                                featureLevels,
                                numFeatureLevels,
                                D3D11_SDK_VERSION,
                                _d3dDevice.GetAddressOf(),
                                &featureLevel,
                                _d3dContext.GetAddressOf()));

    ComPtr<IDXGIDevice> dxgiDevice;
    HR_CHECK(_d3dDevice.As(&dxgiDevice));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    HR_CHECK(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

    ComPtr<IDXGIFactory> dxgiFactory;
    HR_CHECK(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

    // Create swap chain description
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;                                     // Double buffering
    swapChainDesc.BufferDesc.Width = _viewWidth;
    swapChainDesc.BufferDesc.Height = _viewHeight;
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

    HR_CHECK(dxgiFactory->CreateSwapChain(_d3dDevice.Get(), &swapChainDesc, _swapChain.GetAddressOf()));
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

    HR_CHECK(_d3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(),
                                            vsBlob->GetBufferSize(),
                                            nullptr,
                                            _vertexShader.GetAddressOf()));

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

    HR_CHECK(_d3dDevice->CreatePixelShader(psBlob->GetBufferPointer(),
                                           psBlob->GetBufferSize(),
                                           nullptr,
                                           _pixelShader.GetAddressOf()));

    if (_viewWidth > 0 && _viewHeight > 0) {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = _viewWidth;
        texDesc.Height = _viewHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        HR_CHECK(_d3dDevice->CreateTexture2D(&texDesc, nullptr, _cefViewTexture.GetAddressOf()));

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
    ComPtr<ID3D11Texture2D> backBuffer;
    HR_CHECK(_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
    HR_CHECK(_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, _renderTargetView.GetAddressOf()));
    return true;
}

bool OsrRendererD3D11::createSampler() {
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
    _d3dContext->IASetInputLayout(_inputLayout.Get());
    _d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _d3dContext->VSSetShader(_vertexShader.Get(), nullptr, 0);
    _d3dContext->PSSetShader(_pixelShader.Get(), nullptr, 0);
    _d3dContext->PSSetSamplers(0, 1, _samplerState.GetAddressOf());

    float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    _d3dContext->OMSetBlendState(_blenderState.Get(), blendFactor, 0xffffffff);
    _d3dContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(_viewWidth);
    viewport.Height = static_cast<float>(_viewHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _d3dContext->RSSetViewports(1, &viewport);
}

void OsrRendererD3D11::onPaint(CefRenderHandler::PaintElementType type,
                               const CefRenderHandler::RectList& dirtyRects,
                               const void* buffer,
                               int width,
                               int height) {
    if (!_d3dDevice || !_d3dContext) {
        LOGE << "onPaint() FAILED - device or context is null";
        return;
    }

    if (type != PET_VIEW) {
        return;
    }

    // Check if texture size needs to be recreated
    if (_cefViewTexture) {
        D3D11_TEXTURE2D_DESC desc;
        _cefViewTexture->GetDesc(&desc);
        if (desc.Width != static_cast<UINT>(width) || desc.Height != static_cast<UINT>(height)) {
            _cefViewTexture.Reset();
            _cefViewShaderResourceView.Reset();
        }
    }

    // Create texture if not exists
    if (!_cefViewTexture) {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        HRESULT hr = _d3dDevice->CreateTexture2D(&texDesc, nullptr, _cefViewTexture.GetAddressOf());
        if (FAILED(hr)) {
            LOGE << "onPaint() CreateTexture2D FAILED hr=0x" << std::hex << hr;
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = _d3dDevice->CreateShaderResourceView(_cefViewTexture.Get(), &srvDesc, _cefViewShaderResourceView.GetAddressOf());
        if (FAILED(hr)) {
            LOGE << "onPaint() CreateShaderResourceView FAILED hr=0x" << std::hex << hr;
            _cefViewTexture.Reset();
            return;
        }
        LOGD << "onPaint() texture created " << width << "x" << height;
    }

    // Update texture data (only update dirty rectangles)
    for (size_t i = 0; i < dirtyRects.size(); ++i) {
        const CefRect& rect = dirtyRects[i];

        int srcPitch = width * 4;
        const unsigned char* srcData = static_cast<const unsigned char*>(buffer);
        srcData += rect.y * srcPitch + rect.x * 4;

        D3D11_BOX destBox = {};
        destBox.left = rect.x;
        destBox.top = rect.y;
        destBox.right = rect.x + rect.width;
        destBox.bottom = rect.y + rect.height;
        destBox.front = 0;
        destBox.back = 1;

        _d3dContext->UpdateSubresource(_cefViewTexture.Get(), 0, &destBox, srcData, srcPitch, 0);
    }
}

void OsrRendererD3D11::onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                          const CefRenderHandler::RectList& dirtyRects,
                                          const CefAcceleratedPaintInfo& info) {
    if (!_d3dDevice || !_d3dContext) {
        LOGE << "onAcceleratedPaint() FAILED - device or context is null";
        return;
    }

    if (type != PET_VIEW) {
        return;
    }

    void* sharedHandle = info.shared_texture_handle;
    if (!sharedHandle) {
        return;
    }

    // Check if shared texture handle changed
    if (_sharedTextureHandle != sharedHandle) {
        _sharedTexture.Reset();
        _sharedTextureSRV.Reset();
        _sharedTextureHandle = nullptr;
    }

    // Open shared texture if needed
    if (!_sharedTexture) {
        ComPtr<ID3D11Device1> device1;
        HRESULT hr = _d3dDevice.As(&device1);
        if (FAILED(hr)) {
            LOGE << "onAcceleratedPaint() QueryInterface ID3D11Device1 FAILED hr=0x" << std::hex << hr;
            return;
        }

        hr = device1->OpenSharedResource1(sharedHandle, IID_PPV_ARGS(&_sharedTexture));
        if (FAILED(hr)) {
            LOGE << "onAcceleratedPaint() OpenSharedResource1 FAILED hr=0x" << std::hex << hr;
            return;
        }

        D3D11_TEXTURE2D_DESC sharedDesc;
        _sharedTexture->GetDesc(&sharedDesc);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = sharedDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = _d3dDevice->CreateShaderResourceView(_sharedTexture.Get(), &srvDesc, _sharedTextureSRV.GetAddressOf());
        if (FAILED(hr)) {
            LOGE << "onAcceleratedPaint() CreateShaderResourceView FAILED hr=0x" << std::hex << hr;
            _sharedTexture.Reset();
            return;
        }

        _sharedTextureHandle = sharedHandle;
        LOGD << "onAcceleratedPaint() shared texture opened " << sharedDesc.Width << "x" << sharedDesc.Height;
    }
}

void OsrRendererD3D11::render() {
    if (!_d3dContext || !_renderTargetView) {
        return;
    }

    // Set render target and viewport
    _d3dContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(_viewWidth);
    viewport.Height = static_cast<float>(_viewHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _d3dContext->RSSetViewports(1, &viewport);

    // Clear render target
    float clearColor[4] = {0.0f, 0.0f, 0.0f, _transparent ? 0.0f : 1.0f};
    _d3dContext->ClearRenderTargetView(_renderTargetView.Get(), clearColor);

    // Select texture SRV: prefer shared texture (hardware), fallback to local texture (software)
    ID3D11ShaderResourceView* textureSRV = nullptr;
    if (_sharedTextureSRV) {
        textureSRV = _sharedTextureSRV.Get();
    } else if (_cefViewShaderResourceView) {
        textureSRV = _cefViewShaderResourceView.Get();
    }

    // Draw View texture (fullscreen quad)
    if (textureSRV) {
        if (!_cefViewVertexBuffer) {
            createQuadVertexBuffer(-1.0f, 1.0f, 2.0f, -2.0f, _viewWidth, _viewHeight, _cefViewVertexBuffer.GetAddressOf());
        }

        if (_cefViewVertexBuffer) {
            // Set pipeline state
            _d3dContext->IASetInputLayout(_inputLayout.Get());
            _d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            _d3dContext->VSSetShader(_vertexShader.Get(), nullptr, 0);
            _d3dContext->PSSetShader(_pixelShader.Get(), nullptr, 0);
            _d3dContext->PSSetSamplers(0, 1, _samplerState.GetAddressOf());

            float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            _d3dContext->OMSetBlendState(_blenderState.Get(), blendFactor, 0xffffffff);

            // Set vertex buffer and texture
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            _d3dContext->IASetVertexBuffers(0, 1, _cefViewVertexBuffer.GetAddressOf(), &stride, &offset);
            _d3dContext->PSSetShaderResources(0, 1, &textureSRV);

            // Draw
            _d3dContext->Draw(6, 0);
        }
    }

    HRESULT hr = _swapChain->Present(1, 0);
    if (FAILED(hr)) {
        LOGE << "render() Present FAILED hr=0x" << std::hex << hr;
    }
}

void OsrRendererD3D11::setBounds(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    _viewX = x;
    _viewY = y;

    if (width == _viewWidth && height == _viewHeight) {
        return;
    }

    _viewWidth = width;
    _viewHeight = height;

    if (!_swapChain) {
        return;
    }

    _renderTargetView.Reset();
    _d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    _d3dContext->Flush();

    HRESULT hr = _swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    if (FAILED(hr)) {
        handleDeviceLost();
        return;
    }

    createRenderTargetView();
    setupPipeline();

    _cefViewVertexBuffer.Reset();
}

void OsrRendererD3D11::handleDeviceLost() {
    uninitialize();
    initialize();
}

bool OsrRendererD3D11::createQuadVertexBuffer(float x,
                                              float y,
                                              float w,
                                              float h,
                                              int viewWidth,
                                              int viewHeight,
                                              ID3D11Buffer** ppBuffer) {
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

#endif  // defined(_WIN32)
