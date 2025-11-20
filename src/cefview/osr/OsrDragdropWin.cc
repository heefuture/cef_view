// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "OsrDragdropWin.h"

#if defined(CEF_USE_ATL)

#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>

#include <algorithm>
#include <string>

#include "BytesWriteHandler.h"
#include "include/wrapper/cef_helpers.h"
#include "utils/WinUtil.h"

namespace cefview {

namespace {

DWORD DragOperationToDropEffect(CefRenderHandler::DragOperation allowedOps) {
    DWORD effect = DROPEFFECT_NONE;
    if (allowedOps & DRAG_OPERATION_COPY) {
        effect |= DROPEFFECT_COPY;
    }
    if (allowedOps & DRAG_OPERATION_LINK) {
        effect |= DROPEFFECT_LINK;
    }
    if (allowedOps & DRAG_OPERATION_MOVE) {
        effect |= DROPEFFECT_MOVE;
    }
    return effect;
}

CefRenderHandler::DragOperationsMask DropEffectToDragOperation(DWORD effect) {
  DWORD operation = DRAG_OPERATION_NONE;
  if (effect & DROPEFFECT_COPY) {
    operation |= DRAG_OPERATION_COPY;
  }
  if (effect & DROPEFFECT_LINK) {
    operation |= DRAG_OPERATION_LINK;
  }
  if (effect & DROPEFFECT_MOVE) {
    operation |= DRAG_OPERATION_MOVE;
  }
  return static_cast<CefRenderHandler::DragOperationsMask>(operation);
}

CefMouseEvent ToMouseEvent(POINTL p, DWORD keyState, HWND hWnd) {
    CefMouseEvent ev;
    POINT screenPoint = {p.x, p.y};
    ::ScreenToClient(hWnd, &screenPoint);
    ev.x = screenPoint.x;
    ev.y = screenPoint.y;
    ev.modifiers = util::getCefMouseModifiers(keyState);
    return ev;
}

void GetStorageForBytes(STGMEDIUM* storage, const void* data, size_t bytes) {
  HANDLE handle = ::GlobalAlloc(GPTR, static_cast<int>(bytes));
  if (handle) {
    memcpy(handle, data, bytes);
  }

  storage->hGlobal = handle;
  storage->tymed = TYMED_HGLOBAL;
  storage->pUnkForRelease = nullptr;
}

template <typename T>
void GetStorageForString(STGMEDIUM* stgmed, const std::basic_string<T>& data) {
  GetStorageForBytes(
      stgmed, data.c_str(),
      (data.size() + 1) * sizeof(typename std::basic_string<T>::value_type));
}

void GetStorageForFileDescriptor(STGMEDIUM* storage, const std::wstring& fileName) {
    DCHECK(!fileName.empty());
    HANDLE hdata = ::GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));

    FILEGROUPDESCRIPTOR* descriptor = reinterpret_cast<FILEGROUPDESCRIPTOR*>(hdata);
    descriptor->cItems = 1;
    descriptor->fgd[0].dwFlags = FD_LINKUI;
    wcsncpy_s(descriptor->fgd[0].cFileName,
              MAX_PATH,
              fileName.c_str(),
              std::min(fileName.size(), static_cast<size_t>(MAX_PATH - 1u)));

    storage->tymed = TYMED_HGLOBAL;
    storage->hGlobal = hdata;
    storage->pUnkForRelease = nullptr;
}

// Helper method for converting from text/html to MS CF_HTML.
// Documentation for the CF_HTML format is available at
// http://msdn.microsoft.com/en-us/library/aa767917(VS.85).aspx
std::string HtmlToCFHtml(const std::string& html, const std::string& baseUrl) {
    if (html.empty()) {
        return std::string();
    }

#define MAX_DIGITS 10
#define MAKE_NUMBER_FORMAT_1(digits) MAKE_NUMBER_FORMAT_2(digits)
#define MAKE_NUMBER_FORMAT_2(digits) "%0" #digits "u"
#define NUMBER_FORMAT MAKE_NUMBER_FORMAT_1(MAX_DIGITS)

    static const char* header =
        "Version:0.9\r\n"
        "StartHTML:" NUMBER_FORMAT
        "\r\n"
        "EndHTML:" NUMBER_FORMAT
        "\r\n"
        "StartFragment:" NUMBER_FORMAT
        "\r\n"
        "EndFragment:" NUMBER_FORMAT "\r\n";
    static const char* kSourceUrlOrefix = "SourceURL:";

    static const char* kStartMarkup = "<html>\r\n<body>\r\n<!--StartFragment-->";
    static const char* kEndMarkup = "<!--EndFragment-->\r\n</body>\r\n</html>";

    // Calculate offsets
    size_t startHtmlOffset = strlen(header) - strlen(NUMBER_FORMAT) * 4 + MAX_DIGITS * 4;
    if (!baseUrl.empty()) {
        startHtmlOffset += strlen(kSourceUrlOrefix) + baseUrl.length() + 2;  // Add 2 for \r\n.
    }
    size_t startFragmentOffset = startHtmlOffset + strlen(kStartMarkup);
    size_t endFragmentOffset = startFragmentOffset + html.length();
    size_t endHtmlOffset = endFragmentOffset + strlen(kEndMarkup);
    char rawResult[1024];
    _snprintf(
        rawResult, sizeof(1024), header, startHtmlOffset, endHtmlOffset, startFragmentOffset, endFragmentOffset);
    std::string result = rawResult;
    if (!baseUrl.empty()) {
        result.append(kSourceUrlOrefix);
        result.append(baseUrl);
        result.append("\r\n");
    }
    result.append(kStartMarkup);
    result.append(html);
    result.append(kEndMarkup);

#undef MAX_DIGITS
#undef MAKE_NUMBER_FORMAT_1
#undef MAKE_NUMBER_FORMAT_2
#undef NUMBER_FORMAT

    return result;
}

void CFHtmlExtractMetadata(const std::string& cfHtml,
                           std::string* baseUrl,
                           size_t* htmlStart,
                           size_t* fragmentStart,
                           size_t* fragmentEnd) {
    // Obtain baseUrl if present.
    if (baseUrl) {
        static std::string srcUrlStr("SourceURL:");
        size_t lineStart = cfHtml.find(srcUrlStr);
        if (lineStart != std::string::npos) {
            size_t srcEnd = cfHtml.find("\r\n", lineStart);
            size_t srcStart = lineStart + srcUrlStr.length();
            if (srcEnd != std::string::npos && srcStart != std::string::npos) {
                *baseUrl = cfHtml.substr(srcStart, srcEnd - srcStart);
            }
        }
    }

    // Find the markup between "<!--StartFragment-->" and "<!--EndFragment-->".
    // If the comments cannot be found, like copying from OpenOffice Writer,
    // we simply fall back to using StartFragment/EndFragment bytecount values
    // to determine the fragment indexes.
    std::string cfHtmlLower = cfHtml;
    size_t markupStart = cfHtmlLower.find("<html", 0);
    if (htmlStart) {
        *htmlStart = markupStart;
    }
    size_t tagStart = cfHtml.find("<!--StartFragment", markupStart);
    if (tagStart == std::string::npos) {
        static std::string startFragmentStr("StartFragment:");
        size_t startFragmentStart = cfHtml.find(startFragmentStr);
        if (startFragmentStart != std::string::npos) {
            *fragmentStart =
                static_cast<size_t>(atoi(cfHtml.c_str() + startFragmentStart + startFragmentStr.length()));
        }

        static std::string endFragmentStr("EndFragment:");
        size_t endFragmentStart = cfHtml.find(endFragmentStr);
        if (endFragmentStart != std::string::npos) {
            *fragmentEnd =
                static_cast<size_t>(atoi(cfHtml.c_str() + endFragmentStart + endFragmentStr.length()));
        }
    } else {
        *fragmentStart = cfHtml.find('>', tagStart) + 1;
        size_t tagEnd = cfHtml.rfind("<!--EndFragment", std::string::npos);
        *fragmentEnd = cfHtml.rfind('<', tagEnd);
    }
}

void CFHtmlToHtml(const std::string& cfHtml, std::string* html, std::string* baseUrl) {
    size_t fragStart = std::string::npos;
    size_t fragEnd = std::string::npos;

    CFHtmlExtractMetadata(cfHtml, baseUrl, nullptr, &fragStart, &fragEnd);

    if (html && fragStart != std::string::npos && fragEnd != std::string::npos) {
        *html = cfHtml.substr(fragStart, fragEnd - fragStart);
    }
}

DWORD GetMozUrlFormat() {
    static DWORD mozUrlFormat = ::RegisterClipboardFormat(L"text/x-moz-url");
    return mozUrlFormat;
}

DWORD GetHtmlFormat() {
    static DWORD htmlFormat = ::RegisterClipboardFormat(L"HTML Format");
    return htmlFormat;
}

DWORD GetFileDescFormat() {
    static DWORD fileDescFormat = ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    return fileDescFormat;
}

DWORD GetFileContentsFormat() {
    static DWORD fileContentsFormat = ::RegisterClipboardFormat(CFSTR_FILECONTENTS);
    return fileContentsFormat;
}

bool DragDataToDataObject(CefRefPtr<CefDragData> dragData, IDataObject** dataObject) {
    const DWORD mozUrlFormat = GetMozUrlFormat();
    const DWORD htmlFormat = GetHtmlFormat();
    const DWORD fileDescFormat = GetFileDescFormat();
    const DWORD fileContentsFormat = GetFileContentsFormat();
    const int kMaxDataObjects = 10;
    FORMATETC fmtetcs[kMaxDataObjects];
    STGMEDIUM stgmeds[kMaxDataObjects];
    FORMATETC fmtetc = {0, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    int currIndex = 0;
    CefString text = dragData->GetFragmentText();
    if (!text.empty()) {
        fmtetc.cfFormat = CF_UNICODETEXT;
        fmtetcs[currIndex] = fmtetc;
        GetStorageForString(&stgmeds[currIndex], text.ToWString());
        currIndex++;
    }
    if (dragData->IsLink() && !dragData->GetLinkURL().empty()) {
        std::wstring xMozUrlStr = dragData->GetLinkURL().ToWString();
        xMozUrlStr += '\n';
        xMozUrlStr += dragData->GetLinkTitle().ToWString();
        fmtetc.cfFormat = mozUrlFormat;
        fmtetcs[currIndex] = fmtetc;
        GetStorageForString(&stgmeds[currIndex], xMozUrlStr);
        currIndex++;
    }
    CefString html = dragData->GetFragmentHtml();
    if (!html.empty()) {
        CefString baseUrl = dragData->GetFragmentBaseURL();
        std::string cfhtml = HtmlToCFHtml(html.ToString(), baseUrl.ToString());
        fmtetc.cfFormat = htmlFormat;
        fmtetcs[currIndex] = fmtetc;
        GetStorageForString(&stgmeds[currIndex], cfhtml);
        currIndex++;
    }

    size_t bufferSize = dragData->GetFileContents(nullptr);
    if (bufferSize) {
        CefRefPtr<BytesWriteHandler> handler = new BytesWriteHandler(bufferSize);
        CefRefPtr<CefStreamWriter> writer = CefStreamWriter::CreateForHandler(handler.get());
        dragData->GetFileContents(writer);
        DCHECK_EQ(handler->GetDataSize(), static_cast<int64_t>(bufferSize));
        CefString fileName = dragData->GetFileName();
        GetStorageForFileDescriptor(&stgmeds[currIndex], fileName.ToWString());
        fmtetc.cfFormat = fileDescFormat;
        fmtetcs[currIndex] = fmtetc;
        currIndex++;
        GetStorageForBytes(&stgmeds[currIndex], handler->GetData(), handler->GetDataSize());
        fmtetc.cfFormat = fileContentsFormat;
        fmtetcs[currIndex] = fmtetc;
        currIndex++;
    }
    DCHECK_LT(currIndex, kMaxDataObjects);

    CComPtr<DataObjectWin> obj = DataObjectWin::Create(fmtetcs, stgmeds, currIndex);
    (*dataObject) = obj.Detach();
    return true;
}

CefRefPtr<CefDragData> DataObjectToDragData(IDataObject* dataObject) {
    const DWORD mozUrlFormat = GetMozUrlFormat();
    const DWORD htmlFormat = GetHtmlFormat();
    CefRefPtr<CefDragData> dragData = CefDragData::Create();
    IEnumFORMATETC* enumFormats = nullptr;
    HRESULT res = dataObject->EnumFormatEtc(DATADIR_GET, &enumFormats);
    if (res != S_OK) {
        return dragData;
    }
    enumFormats->Reset();
    const int kCelt = 10;

  ULONG celtFetched;
  do {
    celtFetched = kCelt;
    FORMATETC rgelt[kCelt];
    res = enumFormats->Next(kCelt, rgelt, &celtFetched);
    for (unsigned i = 0; i < celtFetched; i++) {
      CLIPFORMAT format = rgelt[i].cfFormat;
      if (!(format == CF_UNICODETEXT || format == CF_TEXT ||
            format == mozUrlFormat || format == htmlFormat ||
            format == CF_HDROP) ||
          rgelt[i].tymed != TYMED_HGLOBAL) {
        continue;
      }
      STGMEDIUM medium;
      if (dataObject->GetData(&rgelt[i], &medium) == S_OK) {
        if (!medium.hGlobal) {
          ReleaseStgMedium(&medium);
          continue;
        }
        void* hGlobal = GlobalLock(medium.hGlobal);
        if (!hGlobal) {
          ReleaseStgMedium(&medium);
          continue;
        }
        if (format == CF_UNICODETEXT) {
          CefString text;
          text.FromWString((std::wstring::value_type*)hGlobal);
          dragData->SetFragmentText(text);
        } else if (format == CF_TEXT) {
          CefString text;
          text.FromString((std::string::value_type*)hGlobal);
          dragData->SetFragmentText(text);
        } else if (format == mozUrlFormat) {
          std::wstring html((std::wstring::value_type*)hGlobal);
          size_t pos = html.rfind('\n');
          CefString url(html.substr(0, pos));
          CefString title(html.substr(pos + 1));
          dragData->SetLinkURL(url);
          dragData->SetLinkTitle(title);
        } else if (format == htmlFormat) {
          std::string cfHtml((std::string::value_type*)hGlobal);
          std::string baseUrl;
          std::string html;
          CFHtmlToHtml(cfHtml, &html, &baseUrl);
          dragData->SetFragmentHtml(html);
          dragData->SetFragmentBaseURL(baseUrl);
        }
        if (format == CF_HDROP) {
          HDROP hdrop = (HDROP)hGlobal;
          const int kMaxFilenameLen = 4096;
          const unsigned num_files =
              DragQueryFileW(hdrop, 0xffffffff, nullptr, 0);
          for (unsigned int x = 0; x < num_files; ++x) {
            wchar_t filename[kMaxFilenameLen];
            if (!DragQueryFileW(hdrop, x, filename, kMaxFilenameLen)) {
              continue;
            }
            WCHAR* name = wcsrchr(filename, '\\');
            dragData->AddFile(filename, (name ? name + 1 : filename));
          }
        }
        if (medium.hGlobal) {
          GlobalUnlock(medium.hGlobal);
        }
        if (format == CF_HDROP) {
          DragFinish((HDROP)hGlobal);
        } else {
          ReleaseStgMedium(&medium);
        }
      }
    }
  } while (res == S_OK);
  enumFormats->Release();
  return dragData;
}

}  // namespace

CComPtr<OsrDropTargetWin> OsrDropTargetWin::Create(OsrDragEvents* callback, HWND hWnd) {
    return CComPtr<OsrDropTargetWin>(new OsrDropTargetWin(callback, hWnd));
}

HRESULT OsrDropTargetWin::DragEnter(IDataObject* dataObject,
                                    DWORD keyState,
                                    POINTL cursorPosition,
                                    DWORD* effect) {
    if (!_callback) {
        return E_UNEXPECTED;
    }

    CefRefPtr<CefDragData> dragData = _currentDragData;
    if (!dragData) {
        dragData = DataObjectToDragData(dataObject);
    }
    CefMouseEvent ev = ToMouseEvent(cursorPosition, keyState, _hWnd);
    CefBrowserHost::DragOperationsMask mask = DropEffectToDragOperation(*effect);
    mask = _callback->onDragEnter(dragData, ev, mask);
    *effect = DragOperationToDropEffect(mask);
    return S_OK;
}

CefBrowserHost::DragOperationsMask OsrDropTargetWin::StartDragging(CefRefPtr<CefBrowser> browser,
                                                                   CefRefPtr<CefDragData> dragData,
                                                                   CefRenderHandler::DragOperationsMask allowedOps,
                                                                   int x,
                                                                   int y) {
    CComPtr<IDataObject> dataObject;
    DWORD resEffect = DROPEFFECT_NONE;
    if (DragDataToDataObject(dragData, &dataObject)) {
        CComPtr<DropSourceWin> dropSource = DropSourceWin::Create();
        DWORD effect = DragOperationToDropEffect(allowedOps);
        _currentDragData = dragData->Clone();
        _currentDragData->ResetFileContents();
        HRESULT res = DoDragDrop(dataObject, dropSource, effect, &resEffect);
        if (res != DRAGDROP_S_DROP) {
            resEffect = DROPEFFECT_NONE;
        }
        _currentDragData = nullptr;
    }
    return DropEffectToDragOperation(resEffect);
}

HRESULT OsrDropTargetWin::DragOver(DWORD keyState, POINTL cursorPosition, DWORD* effect) {
    if (!_callback) {
        return E_UNEXPECTED;
    }
    CefMouseEvent ev = ToMouseEvent(cursorPosition, keyState, _hWnd);
    CefBrowserHost::DragOperationsMask mask = DropEffectToDragOperation(*effect);
    mask = _callback->onDragOver(ev, mask);
    *effect = DragOperationToDropEffect(mask);
    return S_OK;
}

HRESULT OsrDropTargetWin::DragLeave() {
    if (!_callback) {
        return E_UNEXPECTED;
    }
    _callback->onDragLeave();
    return S_OK;
}

HRESULT OsrDropTargetWin::Drop(IDataObject* dataObject,
                               DWORD keyState,
                               POINTL cursorPosition,
                               DWORD* effect) {
    if (!_callback) {
        return E_UNEXPECTED;
    }
    CefMouseEvent ev = ToMouseEvent(cursorPosition, keyState, _hWnd);
    CefBrowserHost::DragOperationsMask mask = DropEffectToDragOperation(*effect);
    mask = _callback->onDrop(ev, mask);
    *effect = DragOperationToDropEffect(mask);
    return S_OK;
}

CComPtr<DropSourceWin> DropSourceWin::Create() {
  return CComPtr<DropSourceWin>(new DropSourceWin());
}

HRESULT DropSourceWin::GiveFeedback(DWORD dwEffect) {
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT DropSourceWin::QueryContinueDrag(BOOL fEscapePressed,
                                         DWORD grfKeyState) {
  if (fEscapePressed) {
    return DRAGDROP_S_CANCEL;
  }

  if (!(grfKeyState & MK_LBUTTON)) {
    return DRAGDROP_S_DROP;
  }

  return S_OK;
}

HRESULT DragEnumFormatEtc::CreateEnumFormatEtc(
    UINT cfmt,
    FORMATETC* afmt,
    IEnumFORMATETC** ppEnumFormatEtc) {
  if (cfmt == 0 || afmt == nullptr || ppEnumFormatEtc == nullptr) {
    return E_INVALIDARG;
  }

  *ppEnumFormatEtc = new DragEnumFormatEtc(afmt, cfmt);

  return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}

HRESULT DragEnumFormatEtc::Next(ULONG celt, FORMATETC* pFormatEtc, ULONG* pceltFetched) {
    ULONG copied = 0;

    // copy the FORMATETC structures into the caller's buffer
    while (_nIndex < _nNumFormats && copied < celt) {
        DeepCopyFormatEtc(&pFormatEtc[copied], &_pFormatEtc[_nIndex]);
        copied++;
        _nIndex++;
    }

    // store result
    if (pceltFetched != nullptr) {
        *pceltFetched = copied;
    }

    // did we copy all that was requested?
    return (copied == celt) ? S_OK : S_FALSE;
}

HRESULT DragEnumFormatEtc::Skip(ULONG celt) {
    _nIndex += celt;
    return (_nIndex <= _nNumFormats) ? S_OK : S_FALSE;
}

HRESULT DragEnumFormatEtc::Reset() {
    _nIndex = 0;
    return S_OK;
}

HRESULT DragEnumFormatEtc::Clone(IEnumFORMATETC** ppEnumFormatEtc) {
    HRESULT hResult;

    // make a duplicate enumerator
    hResult = CreateEnumFormatEtc(_nNumFormats, _pFormatEtc, ppEnumFormatEtc);

    if (hResult == S_OK) {
        // manually set the index state
        reinterpret_cast<DragEnumFormatEtc*>(*ppEnumFormatEtc)->_nIndex = _nIndex;
    }

    return hResult;
}

DragEnumFormatEtc::DragEnumFormatEtc(FORMATETC* pFormatEtc, int nNumFormats) {
    AddRef();

    _nIndex = 0;
    _nNumFormats = nNumFormats;
    _pFormatEtc = new FORMATETC[nNumFormats];

    // make a new copy of each FORMATETC structure
    for (int i = 0; i < nNumFormats; i++) {
        DeepCopyFormatEtc(&_pFormatEtc[i], &pFormatEtc[i]);
    }
}

DragEnumFormatEtc::~DragEnumFormatEtc() {
    // first free any DVTARGETDEVICE structures
    for (ULONG i = 0; i < _nNumFormats; i++) {
        if (_pFormatEtc[i].ptd) {
            CoTaskMemFree(_pFormatEtc[i].ptd);
        }
    }

    // now free the main array
    delete[] _pFormatEtc;
}

void DragEnumFormatEtc::DeepCopyFormatEtc(FORMATETC* dest, FORMATETC* source) {
  // copy the source FORMATETC into dest
  *dest = *source;
  if (source->ptd) {
    // allocate memory for the DVTARGETDEVICE if necessary
    dest->ptd = reinterpret_cast<DVTARGETDEVICE*>(
        CoTaskMemAlloc(sizeof(DVTARGETDEVICE)));

    // copy the contents of the source DVTARGETDEVICE into dest->ptd
    *(dest->ptd) = *(source->ptd);
  }
}

CComPtr<DataObjectWin> DataObjectWin::Create(FORMATETC* fmtetc,
                                             STGMEDIUM* stgmed,
                                             int count) {
  return CComPtr<DataObjectWin>(new DataObjectWin(fmtetc, stgmed, count));
}

HRESULT DataObjectWin::GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pmedium) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::QueryGetData(FORMATETC* pFormatEtc) {
  return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

HRESULT DataObjectWin::GetCanonicalFormatEtc(FORMATETC* pFormatEct,
                                             FORMATETC* pFormatEtcOut) {
  pFormatEtcOut->ptd = nullptr;
  return E_NOTIMPL;
}

HRESULT DataObjectWin::SetData(FORMATETC* pFormatEtc,
                               STGMEDIUM* pMedium,
                               BOOL fRelease) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::DAdvise(FORMATETC* pFormatEtc,
                               DWORD advf,
                               IAdviseSink*,
                               DWORD*) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::DUnadvise(DWORD dwConnection) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::EnumDAdvise(IEnumSTATDATA** ppEnumAdvise) {
  return E_NOTIMPL;
}

HRESULT DataObjectWin::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc) {
    return DragEnumFormatEtc::CreateEnumFormatEtc(_nNumFormats, _pFormatEtc, ppEnumFormatEtc);
}

HRESULT DataObjectWin::GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) {
    int idx;

    // try to match the specified FORMATETC with one of our supported formats
    if ((idx = LookupFormatEtc(pFormatEtc)) == -1) {
        return DV_E_FORMATETC;
    }

    // found a match - transfer data into supplied storage medium
    pMedium->tymed = _pFormatEtc[idx].tymed;
    pMedium->pUnkForRelease = nullptr;

    // copy the data into the caller's storage medium
    switch (_pFormatEtc[idx].tymed) {
    case TYMED_HGLOBAL:
        pMedium->hGlobal = DupGlobalMem(_pStgMedium[idx].hGlobal);
        break;

    default:
        return DV_E_FORMATETC;
    }
    return S_OK;
}

HGLOBAL DataObjectWin::DupGlobalMem(HGLOBAL hMem) {
    DWORD len = GlobalSize(hMem);
    PVOID source = GlobalLock(hMem);
    PVOID dest = GlobalAlloc(GMEM_FIXED, len);

    memcpy(dest, source, len);
    GlobalUnlock(hMem);
    return dest;
}

int DataObjectWin::LookupFormatEtc(FORMATETC* pFormatEtc) {
    // check each of our formats in turn to see if one matches
    for (int i = 0; i < _nNumFormats; i++) {
        if ((_pFormatEtc[i].tymed & pFormatEtc->tymed) && _pFormatEtc[i].cfFormat == pFormatEtc->cfFormat &&
            _pFormatEtc[i].dwAspect == pFormatEtc->dwAspect) {
            // return index of stored format
            return i;
        }
    }

    // error, format not found
    return -1;
}

DataObjectWin::DataObjectWin(FORMATETC* fmtetc, STGMEDIUM* stgmed, int count)
    : _refCount(0) {
    _nNumFormats = count;

    _pFormatEtc = new FORMATETC[count];
    _pStgMedium = new STGMEDIUM[count];

    for (int i = 0; i < count; i++) {
        _pFormatEtc[i] = fmtetc[i];
        _pStgMedium[i] = stgmed[i];
    }
}

}  // namespace cefview

#endif  // defined(CEF_USE_ATL)