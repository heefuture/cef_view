/**
 * @file OsrRendererGL.cpp
 * @brief OpenGL 3.3 off-screen renderer implementation
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#include "OsrRendererGL.h"

#include <cstring>
#include <cmath>

#include <utils/LogUtil.h>

#if defined(WIN32)
#include <windows.h>
#include <gl/GL.h>

// OpenGL 3.3 function pointers (loaded dynamically on Windows)
typedef void (APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void (APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint* arrays);
typedef void (APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void (APIENTRY* PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);
typedef void (APIENTRY* PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size, const void* data, GLenum usage);
typedef GLuint (APIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const char** string, const GLint* length);
typedef void (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void (APIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const char* name);
typedef void (APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void (APIENTRY* PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);

// WGL extension function pointers
typedef HGLRC (WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int* attribList);
typedef BOOL (WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);

// GL constants
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_TEXTURE0 0x84C0
#define GL_BGRA 0x80E1
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_CLAMP_TO_EDGE 0x812F

// WGL constants
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023

// Global function pointers
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
static PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
static PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
static PFNGLBUFFERDATAPROC glBufferData = nullptr;
static PFNGLCREATESHADERPROC glCreateShader = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
static PFNGLDELETESHADERPROC glDeleteShader = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
static PFNGLATTACHSHADERPROC glAttachShader = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
static PFNGLUNIFORM1IPROC glUniform1i = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
static PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;

static bool g_glFunctionsLoaded = false;

static bool LoadGLFunctions() {
    if (g_glFunctionsLoaded) {
        return true;
    }

    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
    glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

    g_glFunctionsLoaded = (glGenVertexArrays && glBindVertexArray && glGenBuffers &&
                           glBindBuffer && glBufferData && glCreateShader &&
                           glShaderSource && glCompileShader && glCreateProgram &&
                           glAttachShader && glLinkProgram && glUseProgram &&
                           glGetUniformLocation && glUniformMatrix4fv && glUniform1i &&
                           glEnableVertexAttribArray && glVertexAttribPointer && glActiveTexture);

    return g_glFunctionsLoaded;
}

#elif defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#endif

namespace cefview {

// Vertex shader source (OpenGL 3.3)
static const char* g_vertexShaderSource = R"(
#version 330 core
out vec2 texCoord;
uniform mat4 transform;
void main() {
    float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);
    float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);
    vec4 pos = vec4(-1.0f + x * 2.0f, -1.0f + y * 2.0f, 0.0f, 1.0f);
    gl_Position = transform * pos;
    texCoord = vec2(x, 1.0f - y);
}
)";

// Fragment shader source (OpenGL 3.3)
static const char* g_fragmentShaderSource = R"(
#version 330 core
out vec4 fragColor;
in vec2 texCoord;
uniform sampler2D tex;
void main() {
    fragColor = texture(tex, texCoord);
}
)";

OsrRendererGL::OsrRendererGL(NativeWindowHandle hwnd, int width, int height, bool transparent)
    : OsrRenderer(transparent)
#if defined(WIN32)
    , _hwnd(hwnd)
#elif defined(__APPLE__)
    , _nsView(hwnd)
#else
    , _window(hwnd)
#endif
{
    _viewWidth = width;
    _viewHeight = height;
}

OsrRendererGL::~OsrRendererGL() {
    uninitialize();
}

bool OsrRendererGL::initialize() {
    LOGD << "OsrRendererGL::initialize() called";

    if (!createGLContext()) {
        LOGE << "createGLContext() FAILED";
        return false;
    }
    LOGD << "createGLContext() OK";

    makeCurrent();

#if defined(WIN32)
    if (!LoadGLFunctions()) {
        LOGE << "LoadGLFunctions() FAILED";
        destroyGLContext();
        return false;
    }
    LOGD << "LoadGLFunctions() OK";
#endif

    if (!createShaderProgram()) {
        LOGE << "createShaderProgram() FAILED";
        destroyGLContext();
        return false;
    }
    LOGD << "createShaderProgram() OK";

    if (!createTexture()) {
        LOGE << "createTexture() FAILED";
        destroyGLContext();
        return false;
    }
    LOGD << "createTexture() OK";

    // Create VAO and VBO
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

    // Set clear color
    if (_transparent) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }

    _initialized = true;
    LOGI << "OsrRendererGL initialized successfully";
    return true;
}

void OsrRendererGL::uninitialize() {
    if (!_initialized) {
        return;
    }

    makeCurrent();

    if (_textureId != 0) {
        glDeleteTextures(1, &_textureId);
        _textureId = 0;
    }

    if (_vbo != 0) {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_vao != 0) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }

    if (_shaderProgram != 0) {
        glDeleteProgram(_shaderProgram);
        _shaderProgram = 0;
    }

    destroyGLContext();
    _initialized = false;
}

bool OsrRendererGL::createGLContext() {
#if defined(WIN32)
    _hdc = GetDC(_hwnd);
    if (!_hdc) {
        LOGE << "GetDC() FAILED";
        return false;
    }

    // Create a dummy context to load WGL extensions
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(_hdc, &pfd);
    if (pixelFormat == 0) {
        LOGE << "ChoosePixelFormat() FAILED";
        ReleaseDC(_hwnd, _hdc);
        _hdc = nullptr;
        return false;
    }

    if (!SetPixelFormat(_hdc, pixelFormat, &pfd)) {
        LOGE << "SetPixelFormat() FAILED";
        ReleaseDC(_hwnd, _hdc);
        _hdc = nullptr;
        return false;
    }

    HGLRC tempContext = wglCreateContext(_hdc);
    if (!tempContext) {
        LOGE << "wglCreateContext() FAILED";
        ReleaseDC(_hwnd, _hdc);
        _hdc = nullptr;
        return false;
    }

    wglMakeCurrent(_hdc, tempContext);

    // Load WGL extensions
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    if (wglCreateContextAttribsARB) {
        // Create OpenGL 3.3 core context
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        _hglrc = wglCreateContextAttribsARB(_hdc, nullptr, attribs);
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(tempContext);

    if (!_hglrc) {
        LOGE << "wglCreateContextAttribsARB() FAILED, falling back to legacy context";
        _hglrc = wglCreateContext(_hdc);
        if (!_hglrc) {
            LOGE << "wglCreateContext() FAILED";
            ReleaseDC(_hwnd, _hdc);
            _hdc = nullptr;
            return false;
        }
    }

    return true;
#elif defined(__APPLE__)
    // Mac implementation would use NSOpenGLContext
    // Not implemented in this version
    return false;
#else
    // Linux implementation would use GLX
    // Not implemented in this version
    return false;
#endif
}

void OsrRendererGL::destroyGLContext() {
#if defined(WIN32)
    if (_hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(_hglrc);
        _hglrc = nullptr;
    }
    if (_hdc) {
        ReleaseDC(_hwnd, _hdc);
        _hdc = nullptr;
    }
#elif defined(__APPLE__)
    // Mac cleanup
#else
    // Linux cleanup
#endif
}

void OsrRendererGL::makeCurrent() {
#if defined(WIN32)
    if (_hdc && _hglrc) {
        wglMakeCurrent(_hdc, _hglrc);
    }
#elif defined(__APPLE__)
    // Mac make current
#else
    // Linux make current
#endif
}

void OsrRendererGL::swapBuffers() {
#if defined(WIN32)
    if (_hdc) {
        SwapBuffers(_hdc);
    }
#elif defined(__APPLE__)
    // Mac swap buffers
#else
    // Linux swap buffers
#endif
}

bool OsrRendererGL::createShaderProgram() {
    GLint success = 0;
    char infoLog[512] = {};

    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &g_vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        LOGE << "Vertex shader compile error: " << infoLog;
        glDeleteShader(vertexShader);
        return false;
    }

    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &g_fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        LOGE << "Fragment shader compile error: " << infoLog;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Create shader program
    _shaderProgram = glCreateProgram();
    glAttachShader(_shaderProgram, vertexShader);
    glAttachShader(_shaderProgram, fragmentShader);
    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(_shaderProgram, 512, nullptr, infoLog);
        LOGE << "Shader program link error: " << infoLog;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(_shaderProgram);
        _shaderProgram = 0;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations
    glUseProgram(_shaderProgram);
    _transformLoc = glGetUniformLocation(_shaderProgram, "transform");
    glUniform1i(glGetUniformLocation(_shaderProgram, "tex"), 0);

    return true;
}

bool OsrRendererGL::createTexture() {
    glGenTextures(1, &_textureId);
    if (_textureId == 0) {
        LOGE << "glGenTextures() FAILED";
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, _textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void OsrRendererGL::setBounds(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    _viewX = x;
    _viewY = y;
    _viewWidth = width;
    _viewHeight = height;
}

void OsrRendererGL::onPaint(CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height) {
    if (!_initialized) {
        return;
    }

    if (type != PET_VIEW) {
        return;
    }

    makeCurrent();

    glBindTexture(GL_TEXTURE_2D, _textureId);

    // Check if we need to resize texture
    static int lastWidth = 0;
    static int lastHeight = 0;

    if (width != lastWidth || height != lastHeight) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
        lastWidth = width;
        lastHeight = height;
        LOGD << "onPaint() texture resized " << width << "x" << height;
    } else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
        for (const auto& rect : dirtyRects) {
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect.x);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, rect.y);
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width, rect.height,
                            GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
        }
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    }
}

void OsrRendererGL::onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                       const CefRenderHandler::RectList& dirtyRects,
                                       const CefAcceleratedPaintInfo& info) {
    // OnAcceleratedPaint is not supported on Windows with OpenGL
    // It only works on Mac with IOSurface textures
}

void OsrRendererGL::render() {
    if (!_initialized) {
        return;
    }

    makeCurrent();

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, _viewWidth, _viewHeight);

    // Identity transform matrix
    float transform[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    if (_transparent) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureId);

    glUseProgram(_shaderProgram);
    glUniformMatrix4fv(_transformLoc, 1, GL_FALSE, transform);

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (_transparent) {
        glDisable(GL_BLEND);
    }

    swapBuffers();
}

}  // namespace cefview
