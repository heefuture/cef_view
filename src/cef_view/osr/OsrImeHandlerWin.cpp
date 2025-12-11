/**
 * @file        OsrImeHandlerWin.cpp
 * @brief       Implementation of Windows IME handler for OSR mode
 * @version     1.0
 * @author      cef_view project (based on Chromium ui/base/ime/win/imm32_manager.cc)
 * @date        2025-11-19
 */
#include "OsrImeHandlerWin.h"

#include <msctf.h>
#include <windowsx.h>

#include "include/base/cef_build.h"

#define ColorUNDERLINE 0xFF000000  // Black SkColor value for underline, same as Blink.
#define ColorBKCOLOR 0x00000000    // White SkColor value for background, same as Blink.

namespace cefview {

namespace {

// Determines whether or not the given attribute represents a selection
bool IsSelectionAttribute(char attribute) {
    return (attribute == ATTR_TARGET_CONVERTED || attribute == ATTR_TARGET_NOTCONVERTED);
}

// Helper function for OsrImeHandlerWin::GetCompositionInfo() method,
// to get the target range that's selected by the user in the current
// composition string.
void GetCompositionSelectionRange(HIMC imc,
                                  uint32_t* targetStart,
                                  uint32_t* targetEnd) {
    uint32_t attributeSize = ::ImmGetCompositionString(imc, GCS_COMPATTR, nullptr, 0);
    if (attributeSize > 0) {
        uint32_t start = 0;
        uint32_t end = 0;
        std::vector<char> attributeData(attributeSize);

        ::ImmGetCompositionString(imc, GCS_COMPATTR, &attributeData[0], attributeSize);
        for (start = 0; start < attributeSize; ++start) {
            if (IsSelectionAttribute(attributeData[start])) {
                break;
            }
        }
        for (end = start; end < attributeSize; ++end) {
            if (!IsSelectionAttribute(attributeData[end])) {
                break;
            }
        }

        *targetStart = start;
        *targetEnd = end;
    }
}

// Helper function for OsrImeHandlerWin::GetCompositionInfo() method, to get
// underlines information of the current composition string.
void GetCompositionUnderlines(HIMC imc,
                              uint32_t targetStart,
                              uint32_t targetEnd,
                              std::vector<CefCompositionUnderline>& underlines) {
    int clauseSize = ::ImmGetCompositionString(imc, GCS_COMPCLAUSE, nullptr, 0);
    int clauseLength = clauseSize / sizeof(uint32_t);
    if (clauseLength) {
        std::vector<uint32_t> clauseData(clauseLength);

        ::ImmGetCompositionString(imc, GCS_COMPCLAUSE, &clauseData[0], clauseSize);
        for (int i = 0; i < clauseLength - 1; ++i) {
            cef_composition_underline_t underline = {};
            underline.range.from = clauseData[i];
            underline.range.to = clauseData[i + 1];
            underline.color = ColorUNDERLINE;
            underline.background_color = ColorBKCOLOR;
            underline.thick = 0;

            // Use thick underline for the target clause.
            if (underline.range.from >= targetStart && underline.range.to <= targetEnd) {
                underline.thick = 1;
            }
            underlines.push_back(underline);
        }
    }
}

}  // namespace

OsrImeHandlerWin::OsrImeHandlerWin(HWND hwnd)
    : _inputLanguageId(LANG_USER_DEFAULT)
    , _cursorIndex(std::numeric_limits<uint32_t>::max())
    , _hwnd(hwnd) {
    _imeRect = {-1, -1, 0, 0};
}

OsrImeHandlerWin::~OsrImeHandlerWin() {
    destroyImeWindow();
}

void OsrImeHandlerWin::setInputLanguage() {
    // Retrieve the current input language from the system's keyboard layout.
    // Using GetKeyboardLayoutName instead of GetKeyboardLayout, because
    // the language from GetKeyboardLayout is the language under where the
    // keyboard layout is installed. And the language from GetKeyboardLayoutName
    // indicates the language of the keyboard layout itself.
    // See crbug.com/344834.
    WCHAR keyboardLayout[KL_NAMELENGTH];
    if (::GetKeyboardLayoutNameW(keyboardLayout)) {
        _inputLanguageId = static_cast<LANGID>(_wtoi(&keyboardLayout[KL_NAMELENGTH >> 1]));
    } else {
        _inputLanguageId = 0x0409;  // Fallback to en-US.
    }
}

void OsrImeHandlerWin::createImeWindow() {
    // Chinese/Japanese IMEs somehow ignore function calls to
    // ::ImmSetCandidateWindow(), and use the position of the current system
    // caret instead -::GetCaretPos().
    // Therefore, we create a temporary system caret for Chinese IMEs and use
    // it during this input context.
    // Since some third-party Japanese IME also uses ::GetCaretPos() to determine
    // their window position, we also create a caret for Japanese IMEs.
    if (PRIMARYLANGID(_inputLanguageId) == LANG_CHINESE || PRIMARYLANGID(_inputLanguageId) == LANG_JAPANESE) {
        if (!_systemCaret) {
            if (::CreateCaret(_hwnd, nullptr, 1, 1)) {
                _systemCaret = true;
            }
        }
    }
}

void OsrImeHandlerWin::destroyImeWindow() {
    // Destroy the system caret if we have created for this IME input context.
    if (_systemCaret) {
        ::DestroyCaret();
        _systemCaret = false;
    }
}

void OsrImeHandlerWin::moveImeWindow() {
    // Does nothing when the target window has no input focus.
    if (GetFocus() != _hwnd) {
        return;
    }

    CefRect rc = _imeRect;
    uint32_t location = _cursorIndex;

    // If location is not specified fall back to the composition range start.
    if (location == std::numeric_limits<uint32_t>::max()) {
        location = _compositionRange.from;
    }

    // Offset location by the composition range start if required.
    if (location >= _compositionRange.from) {
        location -= _compositionRange.from;
    }

    if (location < _compositionBounds.size()) {
        rc = _compositionBounds[location];
    } else {
        return;
    }

    HIMC imc = ::ImmGetContext(_hwnd);
    if (imc) {
        const int kCaretMargin = 1;
        if (PRIMARYLANGID(_inputLanguageId) == LANG_CHINESE) {
            // Chinese IMEs ignore function calls to ::ImmSetCandidateWindow()
            // when a user disables TSF (Text Service Framework) and CUAS (Cicero
            // Unaware Application Support).
            // On the other hand, when a user enables TSF and CUAS, Chinese IMEs
            // ignore the position of the current system caret and use the
            // parameters given to ::ImmSetCandidateWindow() with its 'dwStyle'
            // parameter CFS_CANDIDATEPOS.
            // Therefore, we do not only call ::ImmSetCandidateWindow() but also
            // set the positions of the temporary system caret if it exists.
            CANDIDATEFORM candidatePosition = {0, CFS_CANDIDATEPOS, {rc.x, rc.y}, {0, 0, 0, 0}};
            ::ImmSetCandidateWindow(imc, &candidatePosition);
        }
        if (_systemCaret) {
            switch (PRIMARYLANGID(_inputLanguageId)) {
            case LANG_JAPANESE:
                ::SetCaretPos(rc.x, rc.y + rc.height);
                break;
            default:
                ::SetCaretPos(rc.x, rc.y);
                break;
            }
        }

        if (PRIMARYLANGID(_inputLanguageId) == LANG_KOREAN) {
            // Korean IMEs require the lower-left corner of the caret to move their
            // candidate windows.
            rc.y += kCaretMargin;
        }

        // Japanese IMEs and Korean IMEs also use the rectangle given to
        // ::ImmSetCandidateWindow() with its 'dwStyle' parameter CFS_EXCLUDE
        // Therefore, we also set this parameter here.
        CANDIDATEFORM excludeRectangle = {0, CFS_EXCLUDE, {rc.x, rc.y}, {rc.x, rc.y, rc.x + rc.width, rc.y + rc.height}};
        ::ImmSetCandidateWindow(imc, &excludeRectangle);

        ::ImmReleaseContext(_hwnd, imc);
    }
}

void OsrImeHandlerWin::cleanupComposition() {
    // Notify the IMM attached to the given window to complete the ongoing
    // composition (when given window is de-activated while composing and
    // re-activated) and reset the composition status.
    if (_isComposing) {
        HIMC imc = ::ImmGetContext(_hwnd);
        if (imc) {
            ::ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            ::ImmReleaseContext(_hwnd, imc);
        }
        resetComposition();
    }
}

void OsrImeHandlerWin::resetComposition() {
    // Reset the composition status.
    _isComposing = false;
    _cursorIndex = std::numeric_limits<uint32_t>::max();
}

void OsrImeHandlerWin::getCompositionInfo(HIMC imc,
                                          LPARAM lparam,
                                          CefString& compositionText,
                                          std::vector<CefCompositionUnderline>& underlines,
                                          int& compositionStart) {
    // We only care about GCS_COMPATTR, GCS_COMPCLAUSE and GCS_CURSORPOS, and
    // convert them into underlines and selection range respectively.
    underlines.clear();

    uint32_t length = static_cast<uint32_t>(compositionText.length());

    // Find out the range selected by the user.
    uint32_t targetStart = length;
    uint32_t targetEnd = length;
    if (lparam & GCS_COMPATTR) {
        GetCompositionSelectionRange(imc, &targetStart, &targetEnd);
    }

    // Retrieve the selection range information. If CS_NOMOVECARET is specified
    // it means the cursor should not be moved and we therefore place the caret at
    // the beginning of the composition string. Otherwise we should honour the
    // GCS_CURSORPOS value if it's available.
    // TODO(suzhe): Due to a bug in WebKit we currently can't use selection range
    // with composition string.
    // See: https://bugs.webkit.org/show_bug.cgi?id=40805
    if (!(lparam & CS_NOMOVECARET) && (lparam & GCS_CURSORPOS)) {
        // IMM32 does not support non-zero-width selection in a composition. So
        // always use the caret position as selection range.
        int cursor = ::ImmGetCompositionString(imc, GCS_CURSORPOS, nullptr, 0);
        compositionStart = cursor;
    } else {
        compositionStart = 0;
    }

    // Retrieve the clause segmentations and convert them to underlines.
    if (lparam & GCS_COMPCLAUSE) {
        GetCompositionUnderlines(imc, targetStart, targetEnd, underlines);
    }

    // Set default underlines in case there is no clause information.
    if (!underlines.size()) {
        CefCompositionUnderline underline;
        underline.color = ColorUNDERLINE;
        underline.background_color = ColorBKCOLOR;
        if (targetStart > 0) {
            underline.range.from = 0;
            underline.range.to = targetStart;
            underline.thick = 0;
            underlines.push_back(underline);
        }
        if (targetEnd > targetStart) {
            underline.range.from = targetStart;
            underline.range.to = targetEnd;
            underline.thick = 1;
            underlines.push_back(underline);
        }
        if (targetEnd < length) {
            underline.range.from = targetEnd;
            underline.range.to = length;
            underline.thick = 0;
            underlines.push_back(underline);
        }
    }
}

bool OsrImeHandlerWin::getString(HIMC imc, WPARAM lparam, int type, CefString& result) {
    if (!(lparam & type)) {
        return false;
    }
    LONG stringSize = ::ImmGetCompositionString(imc, type, nullptr, 0);
    if (stringSize <= 0) {
        return false;
    }

    // For trailing nullptr - ImmGetCompositionString excludes that.
    stringSize += sizeof(WCHAR);

    std::vector<wchar_t> buffer(stringSize);
    ::ImmGetCompositionString(imc, type, &buffer[0], stringSize);
    result.FromWString(&buffer[0]);
    return true;
}

bool OsrImeHandlerWin::getResult(LPARAM lparam, CefString& result) {
    bool ret = false;
    HIMC imc = ::ImmGetContext(_hwnd);
    if (imc) {
        ret = getString(imc, lparam, GCS_RESULTSTR, result);
        ::ImmReleaseContext(_hwnd, imc);
    }
    return ret;
}

bool OsrImeHandlerWin::getComposition(LPARAM lparam,
                                      CefString& compositionText,
                                      std::vector<CefCompositionUnderline>& underlines,
                                      int& compositionStart) {
    bool ret = false;
    HIMC imc = ::ImmGetContext(_hwnd);
    if (imc) {
        // Copy the composition string to the CompositionText object.
        ret = getString(imc, lparam, GCS_COMPSTR, compositionText);

        if (ret) {
            // Retrieve the composition underlines and selection range information.
            getCompositionInfo(imc, lparam, compositionText, underlines, compositionStart);

            // Mark that there is an ongoing composition.
            _isComposing = true;
        }

        ::ImmReleaseContext(_hwnd, imc);
    }
    return ret;
}

void OsrImeHandlerWin::disableIME() {
    cleanupComposition();
    ::ImmAssociateContextEx(_hwnd, nullptr, 0);
}

void OsrImeHandlerWin::cancelIME() {
    if (_isComposing) {
        HIMC imc = ::ImmGetContext(_hwnd);
        if (imc) {
            ::ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ::ImmReleaseContext(_hwnd, imc);
        }
        resetComposition();
    }
}

void OsrImeHandlerWin::enableIME() {
    // Load the default IME context.
    ::ImmAssociateContextEx(_hwnd, nullptr, IACE_DEFAULT);
}

void OsrImeHandlerWin::updateCaretPosition(uint32_t index) {
    // Save the caret position.
    _cursorIndex = index;
    // Move the IME window.
    moveImeWindow();
}

void OsrImeHandlerWin::changeCompositionRange(const CefRange& selectionRange, const std::vector<CefRect>& bounds) {
    _compositionRange = selectionRange;
    _compositionBounds = bounds;
    moveImeWindow();
}

}  // namespace cefview
