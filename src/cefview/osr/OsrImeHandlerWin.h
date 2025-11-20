/**
 * @file        OsrImeHandlerWin.h
 * @brief       Windows IME handler for OSR mode
 * @version     1.0
 * @author      cef_view project
 * @date        2025-11-19
 */
#ifndef OSR_IME_HANDLER_WIN_H_
#define OSR_IME_HANDLER_WIN_H_
#pragma once

#include <windows.h>

#include <vector>

#include "include/internal/cef_types_wrappers.h"

namespace cefview {

/**
 * @class OsrImeHandlerWin
 * @brief Handles IME for the native parent window that hosts an off-screen browser
 *
 * This object is only accessed on the CEF UI thread.
 */
class OsrImeHandlerWin {
public:
    explicit OsrImeHandlerWin(HWND hwnd);
    virtual ~OsrImeHandlerWin();

    /**
     * @brief Retrieves whether or not there is an ongoing composition
     * @return true if composing, false otherwise
     */
    bool isComposing() const { return _isComposing; }

    /**
     * @brief Retrieves the input language from Windows and update it
     */
    void setInputLanguage();

    /**
     * @brief Creates the IME caret windows if required
     */
    void createImeWindow();

    /**
     * @brief Destroys the IME caret windows
     */
    void destroyImeWindow();

    /**
     * @brief Cleans up all resources and reset composition status
     */
    void cleanupComposition();

    /**
     * @brief Resets the composition status and cancels ongoing composition
     */
    void resetComposition();

    /**
     * @brief Retrieves a composition result of the ongoing composition
     * @param lparam LPARAM from WM_IME_COMPOSITION message
     * @param result Output string result
     * @return true if result retrieved successfully
     */
    bool getResult(LPARAM lparam, CefString& result);

    /**
     * @brief Retrieves the current composition status
     * @param lparam LPARAM from WM_IME_COMPOSITION message
     * @param compositionText Output composition text
     * @param underlines Output underline information
     * @param compositionStart Output composition start position
     * @return true if composition retrieved successfully
     */
    bool getComposition(LPARAM lparam,
                        CefString& compositionText,
                        std::vector<CefCompositionUnderline>& underlines,
                        int& compositionStart);

    /**
     * @brief Enables the IME attached to the given window
     */
    virtual void enableIME();

    /**
     * @brief Disables the IME attached to the given window
     */
    virtual void disableIME();

    /**
     * @brief Cancels an ongoing composition of the IME
     */
    virtual void cancelIME();

    /**
     * @brief Updates the IME caret position
     * @param index Cursor index in composition string
     */
    void updateCaretPosition(uint32_t index);

    /**
     * @brief Updates the composition range
     * @param selectionRange Range of selected characters
     * @param characterBounds Bounds of each character in view device coordinates
     */
    void changeCompositionRange(const CefRange& selectionRange,
                                const std::vector<CefRect>& characterBounds);

    /**
     * @brief Updates the position of the IME windows
     */
    void moveImeWindow();

private:
    /**
     * @brief Retrieves the composition information
     */
    void getCompositionInfo(HIMC immContext,
                            LPARAM lparam,
                            CefString& compositionText,
                            std::vector<CefCompositionUnderline>& underlines,
                            int& compositionStart);

    /**
     * @brief Retrieves a string from the IMM
     */
    bool getString(HIMC immContext, WPARAM lparam, int type, CefString& result);

    bool _isComposing = false;                      // Whether there is an ongoing composition
    std::vector<CefRect> _compositionBounds;        // Current composition character bounds
    LANGID _inputLanguageId;                        // Current input language ID from Windows
    bool _systemCaret = false;                      // Whether system caret has been created
    CefRect _imeRect;                               // Rectangle of the input caret
    uint32_t _cursorIndex;                          // Current cursor index in composition
    CefRange _compositionRange;                     // Composition range in the string
    HWND _hwnd;                                     // Associated window handle
};

}  // namespace cefview

#endif  // OSR_IME_HANDLER_WIN_H_
