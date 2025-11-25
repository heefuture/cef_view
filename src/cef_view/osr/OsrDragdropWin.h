/**
 * @file        OsrDragdropWin.h
 * @brief       Windows drag and drop support for OSR mode
 * @version     1.0
 * @author      cef_view project
 * @date        2025-11-19
 * @copyright
 */
#ifndef OSRDRAGDROPWIN_H
#define OSRDRAGDROPWIN_H
#pragma once

// When generating projects with CMake the CEF_USE_ATL value will be defined
// automatically if using a supported Visual Studio version. Pass -DUSE_ATL=OFF
// to the CMake command-line to disable use of ATL.
// Uncomment this line to manually enable ATL support.
#define CEF_USE_ATL 1

#if defined(CEF_USE_ATL)

#include <atlcomcli.h>
#include <objidl.h>
#include <stdio.h>

#include "OsrDragdropEvents.h"

namespace cefview {

/**
 * @brief Default QueryInterface implementation macro
 */
#define DEFAULT_QUERY_INTERFACE(__Class)                                         \
    HRESULT __stdcall QueryInterface(const IID& iid, void** object) override {   \
        *object = nullptr;                                                       \
        if (IsEqualIID(iid, IID_IUnknown)) {                                     \
            IUnknown* obj = this;                                                \
            *object = obj;                                                       \
        } else if (IsEqualIID(iid, IID_##__Class)) {                             \
            __Class* obj = this;                                                 \
            *object = obj;                                                       \
        } else {                                                                 \
            return E_NOINTERFACE;                                                \
        }                                                                        \
        AddRef();                                                                \
        return S_OK;                                                             \
    }

/**
 * @brief IUnknown implementation macro
 */
#define IUNKNOWN_IMPLEMENTATION          \
    ULONG __stdcall AddRef() override {  \
        return ++_refCount;              \
    }                                    \
    ULONG __stdcall Release() override { \
        if (--_refCount == 0) {          \
            delete this;                 \
            return 0U;                   \
        }                                \
        return _refCount;                \
    }                                    \
                                         \
protected:                               \
    ULONG _refCount;

/**
 * @brief Drop target implementation for Windows
 */
class OsrDropTargetWin : public IDropTarget {
public:
    /**
     * @brief Create a drop target instance
     * @param callback Event callback handler
     * @param hWnd Window handle
     * @return Drop target instance
     */
    static CComPtr<OsrDropTargetWin> Create(OsrDragEvents* callback, HWND hWnd);

    /**
     * @brief Start dragging operation
     * @param browser Browser instance
     * @param dragData Drag data
     * @param allowedOps Allowed drag operations
     * @param x Mouse X coordinate
     * @param y Mouse Y coordinate
     * @return Resulting drag operation
     */
    CefBrowserHost::DragOperationsMask StartDragging(CefRefPtr<CefBrowser> browser,
                                                     CefRefPtr<CefDragData> dragData,
                                                     CefRenderHandler::DragOperationsMask allowedOps,
                                                     int x,
                                                     int y);

    // IDropTarget implementation:
    HRESULT __stdcall DragEnter(IDataObject* dataObject,
                                DWORD keyState,
                                POINTL cursorPosition,
                                DWORD* effect) override;

    HRESULT __stdcall DragOver(DWORD keyState, POINTL cursorPosition, DWORD* effect) override;

    HRESULT __stdcall DragLeave() override;

    HRESULT __stdcall Drop(IDataObject* dataObject,
                           DWORD keyState,
                           POINTL cursorPosition,
                           DWORD* effect) override;

    DEFAULT_QUERY_INTERFACE(IDropTarget)
    IUNKNOWN_IMPLEMENTATION

protected:
    OsrDropTargetWin(OsrDragEvents* callback, HWND hWnd)
        : _refCount(0)
        , _callback(callback)
        , _hWnd(hWnd) {
    }
    virtual ~OsrDropTargetWin() = default;

private:
    OsrDragEvents* _callback;               ///< Event callback handler
    HWND _hWnd;                             ///< Window handle
    CefRefPtr<CefDragData> _currentDragData; ///< Current drag data
};

/**
 * @brief Drop source implementation for Windows
 */
class DropSourceWin : public IDropSource {
public:
    /**
     * @brief Create a drop source instance
     * @return Drop source instance
     */
    static CComPtr<DropSourceWin> Create();

    // IDropSource implementation:
    HRESULT __stdcall GiveFeedback(DWORD dwEffect) override;

    HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override;

    DEFAULT_QUERY_INTERFACE(IDropSource)
    IUNKNOWN_IMPLEMENTATION

protected:
    explicit DropSourceWin() : _refCount(0) {}
    virtual ~DropSourceWin() = default;
};

/**
 * @brief Format enumerator for drag and drop
 */
class DragEnumFormatEtc : public IEnumFORMATETC {
public:
    /**
     * @brief Create format enumerator
     * @param cfmt Number of formats
     * @param afmt Format array
     * @param ppEnumFormatEtc Output enumerator
     * @return HRESULT
     */
    static HRESULT CreateEnumFormatEtc(UINT cfmt, FORMATETC* afmt, IEnumFORMATETC** ppEnumFormatEtc);

    // IEnumFormatEtc members
    HRESULT __stdcall Next(ULONG celt, FORMATETC* pFormatEtc, ULONG* pceltFetched) override;
    HRESULT __stdcall Skip(ULONG celt) override;
    HRESULT __stdcall Reset() override;
    HRESULT __stdcall Clone(IEnumFORMATETC** ppEnumFormatEtc) override;

    /**
     * @brief Constructor
     * @param pFormatEtc Format array
     * @param nNumFormats Number of formats
     */
    DragEnumFormatEtc(FORMATETC* pFormatEtc, int nNumFormats);

    /**
     * @brief Destructor
     */
    virtual ~DragEnumFormatEtc();

    /**
     * @brief Deep copy FORMATETC structure
     * @param dest Destination
     * @param source Source
     */
    static void DeepCopyFormatEtc(FORMATETC* dest, FORMATETC* source);

    DEFAULT_QUERY_INTERFACE(IEnumFORMATETC)
    IUNKNOWN_IMPLEMENTATION

private:
    ULONG _nIndex;          ///< Current enumerator index
    ULONG _nNumFormats;     ///< Number of FORMATETC members
    FORMATETC* _pFormatEtc; ///< Array of FORMATETC objects
};

/**
 * @brief Data object implementation for drag and drop
 */
class DataObjectWin : public IDataObject {
public:
    /**
     * @brief Create data object instance
     * @param fmtetc Format array
     * @param stgmed Storage medium array
     * @param count Array size
     * @return Data object instance
     */
    static CComPtr<DataObjectWin> Create(FORMATETC* fmtetc, STGMEDIUM* stgmed, int count);

    // IDataObject members
    HRESULT __stdcall GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pmedium) override;
    HRESULT __stdcall QueryGetData(FORMATETC* pFormatEtc) override;
    HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC* pFormatEct, FORMATETC* pFormatEtcOut) override;
    HRESULT __stdcall SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease) override;
    HRESULT __stdcall DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink*, DWORD*) override;
    HRESULT __stdcall DUnadvise(DWORD dwConnection) override;
    HRESULT __stdcall EnumDAdvise(IEnumSTATDATA** ppEnumAdvise) override;
    HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc) override;
    HRESULT __stdcall GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) override;

    DEFAULT_QUERY_INTERFACE(IDataObject)
    IUNKNOWN_IMPLEMENTATION

protected:
    int _nNumFormats;         ///< Number of formats
    FORMATETC* _pFormatEtc;   ///< Format array
    STGMEDIUM* _pStgMedium;   ///< Storage medium array

    /**
     * @brief Duplicate global memory
     * @param hMem Source memory handle
     * @return Duplicated memory handle
     */
    static HGLOBAL DupGlobalMem(HGLOBAL hMem);

    /**
     * @brief Lookup format index
     * @param pFormatEtc Format to lookup
     * @return Format index or -1 if not found
     */
    int LookupFormatEtc(FORMATETC* pFormatEtc);

    /**
     * @brief Constructor
     * @param fmtetc Format array
     * @param stgmed Storage medium array
     * @param count Array size
     */
    explicit DataObjectWin(FORMATETC* fmtetc, STGMEDIUM* stgmed, int count);

    /**
     * @brief Destructor
     */
    virtual ~DataObjectWin() = default;
};

}  // namespace cefview

#endif  // defined(CEF_USE_ATL)

#endif  // OSRDRAGDROPWIN_H
