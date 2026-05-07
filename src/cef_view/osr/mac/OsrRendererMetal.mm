/**
 * @file OsrRendererMetal.mm
 * @brief Metal hardware-accelerated off-screen renderer implementation
 *
 * Copyright
 * Licensed under BSD-style license.
 */
#import "OsrRendererMetal.h"

#if defined(__APPLE__)

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <IOSurface/IOSurface.h>
#import <Cocoa/Cocoa.h>
#import <simd/simd.h>

#include "utils/LogUtil.h"

namespace cefview {

// Fullscreen quad vertex structure
struct MetalVertex {
    simd_float2 position;
    simd_float2 texCoord;
};

// Metal shading language source (inline)
static const char* g_metalShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut vertexShader(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 0.0, 1.0);
    out.texCoord = in.texCoord;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> tex [[texture(0)]],
                               sampler samp [[sampler(0)]]) {
    return tex.sample(samp, in.texCoord);
}
)";

OsrRendererMetal::OsrRendererMetal(void* nsView, int width, int height, bool transparent)
    : OsrRenderer(transparent)
    , _nsView(nsView) {
    _viewWidth = width;
    _viewHeight = height;
}

OsrRendererMetal::~OsrRendererMetal() {
    uninitialize();
}

bool OsrRendererMetal::initialize() {
    LOGD << "OsrRendererMetal::initialize() called";

    if (!createMetalDevice()) {
        LOGE << "createMetalDevice() FAILED";
        return false;
    }
    LOGD << "createMetalDevice() OK";

    if (!createRenderPipeline()) {
        LOGE << "createRenderPipeline() FAILED";
        releaseMetalResources();
        return false;
    }
    LOGD << "createRenderPipeline() OK";

    if (!createVertexBuffer()) {
        LOGE << "createVertexBuffer() FAILED";
        releaseMetalResources();
        return false;
    }
    LOGD << "createVertexBuffer() OK";

    if (!createSamplerState()) {
        LOGE << "createSamplerState() FAILED";
        releaseMetalResources();
        return false;
    }
    LOGD << "createSamplerState() OK";

    _initialized = true;
    LOGI << "OsrRendererMetal initialized successfully";
    return true;
}

void OsrRendererMetal::uninitialize() {
    if (!_initialized) {
        return;
    }
    releaseMetalResources();
    _initialized = false;
}

bool OsrRendererMetal::createMetalDevice() {
    NSView* view = (__bridge NSView*)_nsView;
    if (!view) {
        LOGE << "Invalid NSView";
        return false;
    }

    [view setWantsLayer:YES];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        LOGE << "MTLCreateSystemDefaultDevice() returned nil";
        return false;
    }
    _device = (__bridge_retained void*)device;

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    metalLayer.contentsScale = static_cast<CGFloat>(_deviceScaleFactor);
    // drawableSize must be in physical pixels, i.e. DIP × scale. _viewWidth
    // / _viewHeight are cached in DIP so the caller (CefWebView) can hand
    // us NSView bounds directly.
    metalLayer.drawableSize = CGSizeMake(_viewWidth * _deviceScaleFactor,
                                         _viewHeight * _deviceScaleFactor);

    if (_transparent) {
        metalLayer.opaque = NO;
        view.layer.backgroundColor = [NSColor clearColor].CGColor;
    } else {
        metalLayer.opaque = YES;
    }

    view.layer = metalLayer;
    _metalLayer = (__bridge_retained void*)metalLayer;

    id<MTLCommandQueue> commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        LOGE << "newCommandQueue() returned nil";
        return false;
    }
    _commandQueue = (__bridge_retained void*)commandQueue;

    return true;
}

bool OsrRendererMetal::createRenderPipeline() {
    id<MTLDevice> device = (__bridge id<MTLDevice>)_device;

    NSError* error = nil;
    NSString* shaderSource = [NSString stringWithUTF8String:g_metalShaderSource];
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource
                                                  options:nil
                                                    error:&error];
    if (!library) {
        LOGE << "Metal shader compilation failed: " << [[error description] UTF8String];
        return false;
    }

    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragmentShader"];

    if (!vertexFunc || !fragmentFunc) {
        LOGE << "Failed to get shader functions";
        return false;
    }

    MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[1].offset = sizeof(simd_float2);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.layouts[0].stride = sizeof(MetalVertex);
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFunc;
    pipelineDesc.fragmentFunction = fragmentFunc;
    pipelineDesc.vertexDescriptor = vertexDescriptor;
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    if (_transparent) {
        pipelineDesc.colorAttachments[0].blendingEnabled = YES;
        pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
        pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    }

    id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc
                                                                                      error:&error];
    if (!pipelineState) {
        LOGE << "Failed to create pipeline state: " << [[error description] UTF8String];
        return false;
    }
    _pipelineState = (__bridge_retained void*)pipelineState;

    return true;
}

bool OsrRendererMetal::createVertexBuffer() {
    id<MTLDevice> device = (__bridge id<MTLDevice>)_device;

    // Fullscreen quad: two triangles covering NDC (-1,-1) to (1,1)
    MetalVertex vertices[6] = {
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},  // Bottom-left
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},  // Bottom-right
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},  // Top-left
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},  // Top-left
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},  // Bottom-right
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},  // Top-right
    };

    id<MTLBuffer> buffer = [device newBufferWithBytes:vertices
                                               length:sizeof(vertices)
                                              options:MTLResourceStorageModeShared];
    if (!buffer) {
        LOGE << "Failed to create vertex buffer";
        return false;
    }
    _vertexBuffer = (__bridge_retained void*)buffer;

    return true;
}

bool OsrRendererMetal::createSamplerState() {
    id<MTLDevice> device = (__bridge id<MTLDevice>)_device;

    MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;

    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:samplerDesc];
    if (!sampler) {
        LOGE << "Failed to create sampler state";
        return false;
    }
    _samplerState = (__bridge_retained void*)sampler;

    return true;
}

bool OsrRendererMetal::createSoftwareTexture(int width, int height) {
    id<MTLDevice> device = (__bridge id<MTLDevice>)_device;

    if (_softwareTexture) {
        CFRelease(_softwareTexture);
        _softwareTexture = nullptr;
    }

    MTLTextureDescriptor* texDesc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                          width:static_cast<NSUInteger>(width)
                                                         height:static_cast<NSUInteger>(height)
                                                      mipmapped:NO];
    texDesc.usage = MTLTextureUsageShaderRead;
    texDesc.storageMode = MTLStorageModeShared;

    id<MTLTexture> texture = [device newTextureWithDescriptor:texDesc];
    if (!texture) {
        LOGE << "Failed to create software texture";
        return false;
    }

    _softwareTexture = (__bridge_retained void*)texture;
    _softwareTextureWidth = width;
    _softwareTextureHeight = height;

    LOGD << "Created software texture " << width << "x" << height;
    return true;
}

void OsrRendererMetal::updateSoftwareTexture(const void* buffer,
                                             int width,
                                             int height,
                                             const CefRenderHandler::RectList& dirtyRects) {
    id<MTLTexture> texture = (__bridge id<MTLTexture>)_softwareTexture;
    int bytesPerPixel = 4;
    int bytesPerRow = width * bytesPerPixel;

    for (const auto& rect : dirtyRects) {
        MTLRegion region = MTLRegionMake2D(
            static_cast<NSUInteger>(rect.x),
            static_cast<NSUInteger>(rect.y),
            static_cast<NSUInteger>(rect.width),
            static_cast<NSUInteger>(rect.height));

        const uint8_t* srcData = static_cast<const uint8_t*>(buffer);
        srcData += rect.y * bytesPerRow + rect.x * bytesPerPixel;

        [texture replaceRegion:region
                   mipmapLevel:0
                     withBytes:srcData
                   bytesPerRow:static_cast<NSUInteger>(bytesPerRow)];
    }
}

bool OsrRendererMetal::createTextureFromIOSurface(void* ioSurfaceRef) {
    id<MTLDevice> device = (__bridge id<MTLDevice>)_device;
    IOSurfaceRef ioSurface = static_cast<IOSurfaceRef>(ioSurfaceRef);

    if (_ioSurfaceTexture) {
        CFRelease(_ioSurfaceTexture);
        _ioSurfaceTexture = nullptr;
    }

    size_t width = IOSurfaceGetWidth(ioSurface);
    size_t height = IOSurfaceGetHeight(ioSurface);

    MTLTextureDescriptor* texDesc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                          width:width
                                                         height:height
                                                      mipmapped:NO];
    texDesc.usage = MTLTextureUsageShaderRead;
    texDesc.storageMode = MTLStorageModeShared;

    id<MTLTexture> texture = [device newTextureWithDescriptor:texDesc
                                                    iosurface:ioSurface
                                                        plane:0];
    if (!texture) {
        LOGE << "Failed to create texture from IOSurface";
        return false;
    }

    _ioSurfaceTexture = (__bridge_retained void*)texture;
    LOGD << "Created IOSurface texture " << width << "x" << height;
    return true;
}

void OsrRendererMetal::setBounds(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    _viewX = x;
    _viewY = y;

    if (width != _viewWidth || height != _viewHeight) {
        _pendingWidth = width;
        _pendingHeight = height;
        _hasPendingResize = true;

        // Keep CAMetalLayer.drawableSize = DIP × scale so the backing
        // store matches the physical pixel resolution of the screen.
        // Applied eagerly so -nextDrawable never returns a 0-sized
        // drawable between host NSView resize and the next render()
        // call (views that start detached from the window tree would
        // otherwise render blank on first display).
        if (_metalLayer) {
            CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)_metalLayer;
            metalLayer.drawableSize = CGSizeMake(width * _deviceScaleFactor,
                                                 height * _deviceScaleFactor);
        }
    }
}

void OsrRendererMetal::onPaint(CefRenderHandler::PaintElementType type,
                               const CefRenderHandler::RectList& dirtyRects,
                               const void* buffer,
                               int width,
                               int height) {
    if (!_initialized || type != PET_VIEW) {
        return;
    }

    if (!_softwareTexture || width != _softwareTextureWidth || height != _softwareTextureHeight) {
        if (!createSoftwareTexture(width, height)) {
            LOGE << "createSoftwareTexture() FAILED";
            return;
        }
    }

    updateSoftwareTexture(buffer, width, height, dirtyRects);
}

void OsrRendererMetal::onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                          const CefRenderHandler::RectList& dirtyRects,
                                          const CefAcceleratedPaintInfo& info) {
    if (!_initialized || type != PET_VIEW) {
        return;
    }

    IOSurfaceRef ioSurface = reinterpret_cast<IOSurfaceRef>(info.shared_texture_io_surface);
    if (!ioSurface) {
        LOGE << "onAcceleratedPaint: null IOSurface";
        return;
    }

    if (static_cast<void*>(ioSurface) != _currentIOSurface) {
        if (!createTextureFromIOSurface(static_cast<void*>(ioSurface))) {
            LOGE << "createTextureFromIOSurface() FAILED";
            return;
        }
        _currentIOSurface = static_cast<void*>(ioSurface);
    }
}

void OsrRendererMetal::render() {
    if (!_initialized) {
        return;
    }

    if (_hasPendingResize) {
        applyPendingResize();
    }

    CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)_metalLayer;
    id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
    if (!drawable) {
        return;
    }

    // Select texture: prefer IOSurface (hardware), fallback to software
    id<MTLTexture> texture = nil;
    if (_ioSurfaceTexture) {
        texture = (__bridge id<MTLTexture>)_ioSurfaceTexture;
    } else if (_softwareTexture) {
        texture = (__bridge id<MTLTexture>)_softwareTexture;
    }

    if (!texture) {
        return;
    }

    id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)_commandQueue;
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

    if (_transparent) {
        passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
    } else {
        passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    }

    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];

    [encoder setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)_pipelineState];
    [encoder setVertexBuffer:(__bridge id<MTLBuffer>)_vertexBuffer offset:0 atIndex:0];
    [encoder setFragmentTexture:texture atIndex:0];
    [encoder setFragmentSamplerState:(__bridge id<MTLSamplerState>)_samplerState atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];

    [encoder endEncoding];

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

void OsrRendererMetal::scheduleRender() {
    // CAMetalLayer bypasses AppKit's drawRect: cycle, so setNeedsDisplay
    // does not trigger rendering. Dispatch render directly on the main thread.
    if ([NSThread isMainThread]) {
        render();
    } else {
        dispatch_async(dispatch_get_main_queue(), [this]() {
            render();
        });
    }
}

void OsrRendererMetal::setDeviceScaleFactor(float scaleFactor) {
    if (_deviceScaleFactor == scaleFactor) {
        return;
    }
    _deviceScaleFactor = scaleFactor;

    if (_metalLayer) {
        CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)_metalLayer;
        metalLayer.contentsScale = static_cast<CGFloat>(scaleFactor);
        // drawableSize depends on the scale factor; recompute so the
        // backing store matches the new pixel density (e.g. window
        // dragged between 1x and 2x displays).
        metalLayer.drawableSize = CGSizeMake(_viewWidth * scaleFactor,
                                             _viewHeight * scaleFactor);
    }
}

void OsrRendererMetal::applyPendingResize() {
    if (!_hasPendingResize) {
        return;
    }

    if (_pendingWidth == _viewWidth && _pendingHeight == _viewHeight) {
        _hasPendingResize = false;
        return;
    }

    LOGD << "OsrRendererMetal::applyPendingResize: " << _viewWidth << "x" << _viewHeight
         << " -> " << _pendingWidth << "x" << _pendingHeight;

    _viewWidth = _pendingWidth;
    _viewHeight = _pendingHeight;
    _hasPendingResize = false;

    if (_metalLayer) {
        CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)_metalLayer;
        metalLayer.drawableSize = CGSizeMake(_viewWidth * _deviceScaleFactor,
                                             _viewHeight * _deviceScaleFactor);
    }
}

void OsrRendererMetal::releaseMetalResources() {
    if (_softwareTexture) {
        CFRelease(_softwareTexture);
        _softwareTexture = nullptr;
    }
    _softwareTextureWidth = 0;
    _softwareTextureHeight = 0;

    if (_ioSurfaceTexture) {
        CFRelease(_ioSurfaceTexture);
        _ioSurfaceTexture = nullptr;
    }
    _currentIOSurface = nullptr;

    if (_samplerState) {
        CFRelease(_samplerState);
        _samplerState = nullptr;
    }

    if (_vertexBuffer) {
        CFRelease(_vertexBuffer);
        _vertexBuffer = nullptr;
    }

    if (_pipelineState) {
        CFRelease(_pipelineState);
        _pipelineState = nullptr;
    }

    if (_commandQueue) {
        CFRelease(_commandQueue);
        _commandQueue = nullptr;
    }

    // Release metalLayer before device
    if (_metalLayer) {
        CFRelease(_metalLayer);
        _metalLayer = nullptr;
    }

    if (_device) {
        CFRelease(_device);
        _device = nullptr;
    }
}

}  // namespace cefview

#endif  // defined(__APPLE__)
