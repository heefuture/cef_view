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
#include <fstream>
#include <vector>
// #include <ShlObj.h>

#include <utils/LogUtil.h>

// Helper function to save texture to BMP file for debugging
static bool SaveTextureToBMP(ID3D11Device* device,
                             ID3D11DeviceContext* context,
                             ID3D11Texture2D* texture,
                             const wchar_t* filename) {
    if (!device || !context || !texture) {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);

    // Create staging texture for CPU read
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Copy texture to staging
    context->CopyResource(stagingTexture.Get(), texture);

    // Map staging texture
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return false;
    }

    // Save as 32-bit BMP file (with alpha channel)
    std::wstring bmpFilename = filename;
    bmpFilename += L".bmp";

    std::ofstream file(bmpFilename, std::ios::binary);
    if (!file.is_open()) {
        context->Unmap(stagingTexture.Get(), 0);
        return false;
    }

    // 32-bit BMP: 4 bytes per pixel, already 4-byte aligned
    UINT rowSize = desc.Width * 4;
    UINT imageSize = rowSize * desc.Height;

    // BMP File Header (14 bytes)
    uint8_t bmpFileHeader[14] = {
        'B', 'M',           // Signature
        0, 0, 0, 0,         // File size (filled later)
        0, 0, 0, 0,         // Reserved
        0, 0, 0, 0          // Pixel data offset (filled later)
    };
    // Use BITMAPV4HEADER (108 bytes) for alpha channel support
    uint32_t headerSize = 108;
    uint32_t pixelOffset = 14 + headerSize;
    uint32_t fileSize = pixelOffset + imageSize;
    memcpy(&bmpFileHeader[2], &fileSize, 4);
    memcpy(&bmpFileHeader[10], &pixelOffset, 4);

    // BITMAPV4HEADER (108 bytes) for 32-bit BGRA with alpha
    uint8_t bmpInfoHeader[108] = {0};
    int32_t bmpWidth = static_cast<int32_t>(desc.Width);
    int32_t bmpHeight = static_cast<int32_t>(desc.Height);
    uint16_t planes = 1;
    uint16_t bitsPerPixel = 32;
    uint32_t compression = 3;  // BI_BITFIELDS

    memcpy(&bmpInfoHeader[0], &headerSize, 4);
    memcpy(&bmpInfoHeader[4], &bmpWidth, 4);
    memcpy(&bmpInfoHeader[8], &bmpHeight, 4);
    memcpy(&bmpInfoHeader[12], &planes, 2);
    memcpy(&bmpInfoHeader[14], &bitsPerPixel, 2);
    memcpy(&bmpInfoHeader[16], &compression, 4);
    memcpy(&bmpInfoHeader[20], &imageSize, 4);

    // Color masks for BGRA format (BI_BITFIELDS)
    uint32_t redMask   = 0x00FF0000;  // Red mask
    uint32_t greenMask = 0x0000FF00;  // Green mask
    uint32_t blueMask  = 0x000000FF;  // Blue mask
    uint32_t alphaMask = 0xFF000000;  // Alpha mask
    memcpy(&bmpInfoHeader[40], &redMask, 4);
    memcpy(&bmpInfoHeader[44], &greenMask, 4);
    memcpy(&bmpInfoHeader[48], &blueMask, 4);
    memcpy(&bmpInfoHeader[52], &alphaMask, 4);

    // Color space type: LCS_sRGB
    uint32_t csType = 0x73524742;  // 'sRGB'
    memcpy(&bmpInfoHeader[56], &csType, 4);

    // Write headers
    file.write(reinterpret_cast<char*>(bmpFileHeader), 14);
    file.write(reinterpret_cast<char*>(bmpInfoHeader), headerSize);

    // Write pixel data (BMP: bottom-to-top, BGRA order)
    std::vector<uint8_t> rowBuffer(rowSize, 0);
    for (int y = static_cast<int>(desc.Height) - 1; y >= 0; --y) {
        const uint8_t* srcRow = static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < desc.Width; ++x) {
            // Source: BGRA, BMP 32-bit needs BGRA (same order)
            rowBuffer[x * 4 + 0] = srcRow[x * 4 + 0];  // B
            rowBuffer[x * 4 + 1] = srcRow[x * 4 + 1];  // G
            rowBuffer[x * 4 + 2] = srcRow[x * 4 + 2];  // R
            rowBuffer[x * 4 + 3] = srcRow[x * 4 + 3];  // A
        }
        file.write(reinterpret_cast<char*>(rowBuffer.data()), rowSize);
    }

    file.close();
    context->Unmap(stagingTexture.Get(), 0);

    // Log alpha values of first few pixels for debugging
    const uint8_t* srcData = static_cast<const uint8_t*>(mapped.pData);
    for (int i = 0; i < 5; ++i) {
        uint8_t b = srcData[i * 4 + 0];
        uint8_t g = srcData[i * 4 + 1];
        uint8_t r = srcData[i * 4 + 2];
        uint8_t a = srcData[i * 4 + 3];
        LOGD << "Pixel[" << i << "] BGRA=" << static_cast<int>(b) << "," 
             << static_cast<int>(g) << "," << static_cast<int>(r) << "," << static_cast<int>(a);
    }

    return true;
}

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
    if (!createDeviceAndSwapchain()) {
        LOGE << "createDeviceAndSwapchain() FAILED";
        return false;
    }

    if (!createShaderResource()) {
        LOGE << "createShaderResource() FAILED";
        uninitialize();
        return false;
    }

    if (!createSampler()) {
        LOGE << "createSampler() FAILED";
        uninitialize();
        return false;
    }

    if (!createBlender()) {
        LOGE << "createBlender() FAILED";
        uninitialize();
        return false;
    }

    if (!createRenderTargetView()) {
        LOGE << "createRenderTargetView() FAILED";
        uninitialize();
        return false;
    }

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
    // Release DirectComposition resources before swap chain
    _dcompVisual.Reset();
    _dcompTarget.Reset();
    _dcompDevice.Reset();
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

    ComPtr<IDXGIFactory2> dxgiFactory2;
    HR_CHECK(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory2.GetAddressOf())));

    // Create swap chain description
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
    swapChainDesc1.Width = _viewWidth;
    swapChainDesc1.Height = _viewHeight;
    swapChainDesc1.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc1.Stereo = FALSE;
    swapChainDesc1.SampleDesc.Count = 1;
    swapChainDesc1.SampleDesc.Quality = 0;
    swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc1.BufferCount = 2;
    swapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    // Use premultiplied alpha for transparent window composition with DirectComposition
    swapChainDesc1.AlphaMode = _transparent ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc1.Flags = 0;

    if (_transparent) {
        // For transparent mode, use CreateSwapChainForComposition with DirectComposition
        HR_CHECK(dxgiFactory2->CreateSwapChainForComposition(
            _d3dDevice.Get(),
            &swapChainDesc1,
            nullptr,  // No restrict to output
            _swapChain.GetAddressOf()));

        // Create DirectComposition device and bind swap chain
        HR_CHECK(createDirectComposition());
    } else {
        // For opaque mode, use CreateSwapChainForHwnd
        swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        HR_CHECK(dxgiFactory2->CreateSwapChainForHwnd(
            _d3dDevice.Get(),
            _hwnd,
            &swapChainDesc1,
            nullptr,  // No fullscreen desc
            nullptr,  // No restrict to output
            _swapChain.GetAddressOf()));
    }

    HR_CHECK(dxgiFactory2->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER));

    return true;
}

bool OsrRendererD3D11::createDirectComposition() {
    ComPtr<IDXGIDevice> dxgiDevice;
    HR_CHECK(_d3dDevice.As(&dxgiDevice));

    // Create DirectComposition device
    HR_CHECK(DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(_dcompDevice.GetAddressOf())));

    // Create composition target for the window
    HR_CHECK(_dcompDevice->CreateTargetForHwnd(_hwnd, TRUE, _dcompTarget.GetAddressOf()));

    // Create visual and set swap chain as content
    HR_CHECK(_dcompDevice->CreateVisual(_dcompVisual.GetAddressOf()));
    HR_CHECK(_dcompVisual->SetContent(_swapChain.Get()));

    // Set the visual as root of the composition target
    HR_CHECK(_dcompTarget->SetRoot(_dcompVisual.Get()));

    // Commit the composition
    HR_CHECK(_dcompDevice->Commit());

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
            LOGE << "Vertex shader compilation failed: " << (char*)errorBlob->GetBufferPointer();
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
            LOGE << "Pixel shader compilation failed: " << (char*)errorBlob->GetBufferPointer();
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

        // Initialize with transparent black data if in transparent mode
        D3D11_SUBRESOURCE_DATA* pInitialData = nullptr;
        std::vector<uint32_t> transparentData;
        if (_transparent) {
            // Create transparent black pixels (BGRA format: 0x00000000)
            transparentData.resize(_viewWidth * _viewHeight, 0x00000000);
            D3D11_SUBRESOURCE_DATA initialData = {};
            initialData.pSysMem = transparentData.data();
            initialData.SysMemPitch = _viewWidth * 4;  // 4 bytes per pixel
            initialData.SysMemSlicePitch = 0;
            pInitialData = &initialData;
        }

        HR_CHECK(_d3dDevice->CreateTexture2D(&texDesc, pInitialData, _cefViewTexture.GetAddressOf()));

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
    // Use premultiplied alpha blending for transparent window composition
    // Source is already premultiplied, so use ONE instead of SRC_ALPHA
    blendDesc.RenderTarget[0].SrcBlend = _transparent ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
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

        // // DEBUG: Save shared texture to desktop for alpha inspection
        // wchar_t desktopPath[MAX_PATH];
        // if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        //     std::wstring filePath = desktopPath;
        //     filePath += L"\\cef_shared_texture";
        //     if (SaveTextureToBMP(_d3dDevice.Get(), _d3dContext.Get(), _sharedTexture.Get(), filePath.c_str())) {
        //         LOGI << "Saved CEF shared texture to desktop for alpha inspection";
        //     } else {
        //         LOGE << "Failed to save CEF shared texture";
        //     }
        // }

        _sharedTextureHandle = sharedHandle;
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
    // Note: When no CEF content is available, we only clear to transparent color
    // The DirectComposition will handle the transparency correctly

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

    // Apply DPI scaling for physical pixel dimensions
    int physicalWidth = static_cast<int>(width * _deviceScaleFactor);
    int physicalHeight = static_cast<int>(height * _deviceScaleFactor);

    if (physicalWidth == _viewWidth && physicalHeight == _viewHeight) {
        return;
    }

    _viewWidth = physicalWidth;
    _viewHeight = physicalHeight;

    if (!_swapChain) {
        return;
    }

    _renderTargetView.Reset();
    _d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    _d3dContext->Flush();

    // For transparent mode (DirectComposition), don't use ALLOW_MODE_SWITCH flag
    UINT flags = _transparent ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    HRESULT hr = _swapChain->ResizeBuffers(0, physicalWidth, physicalHeight, DXGI_FORMAT_UNKNOWN, flags);

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
