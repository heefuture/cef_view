/**
 * @file        OsrDragdropEvents.h
 * @brief       OSR drag and drop event interface
 * @version     1.0
 * @author      cef_view project
 * @date        2025-11-19
 * @copyright
 */
#ifndef OSRDRAGDROPEVENTS_H
#define OSRDRAGDROPEVENTS_H
#pragma once

#include "include/cef_render_handler.h"

namespace cefview {

/**
 * @brief Interface for handling OSR drag and drop events
 */
class OsrDragEvents {
public:
    /**
     * @brief Called when drag enters the window
     * @param drag_data Drag data
     * @param ev Mouse event
     * @param effect Drag operation mask
     * @return Allowed drag operations
     */
    virtual CefBrowserHost::DragOperationsMask onDragEnter(CefRefPtr<CefDragData> dragData,
                                                           CefMouseEvent ev,
                                                           CefBrowserHost::DragOperationsMask effect) = 0;

    /**
     * @brief Called when drag is over the window
     * @param ev Mouse event
     * @param effect Drag operation mask
     * @return Allowed drag operations
     */
    virtual CefBrowserHost::DragOperationsMask onDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) = 0;

    /**
     * @brief Called when drag leaves the window
     */
    virtual void onDragLeave() = 0;

    /**
     * @brief Called when drop occurs
     * @param ev Mouse event
     * @param effect Drag operation mask
     * @return Allowed drag operations
     */
    virtual CefBrowserHost::DragOperationsMask onDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) = 0;

protected:
    /**
     * @brief Virtual destructor
     */
    virtual ~OsrDragEvents() = default;
};

}  // namespace cefview

#endif  // OSRDRAGDROPEVENTS_H
