
/**
* @file        CefWebView.h
* @brief       CefWebView 类的定义
*              用于创建和管理 CEF 浏览器视图的类
* @version     1.0
* @author      heefuture
* @date        2025.07.08
* @copyright   Copyright (C) 2025 Tencent. All rights reserved.
*/
#ifndef CEFWEBVIEW_H
#define CEFWEBVIEW_H
#pragma once


#include <memory>
#include <string>
#include <windows.h>
namespace cefview {

class CefViewClientDelegate;
class CefWebView : public std::enable_shared_from_this<CefWebView>
{
public:
    CefWebView(HWND parentHwnd);
    ~CefWebView(void);

    void init();
    void setRect(int left, int top, int width, int height);
    //virtual void HandleMessage(EventArgs& event) override;
    void setVisible(bool bVisible = true);

        /**
    * @brief 加载一个地址
    * @param[in] url 网站地址
    * @return 无
    */
    void loadURL(const std::string& url);

    /**
    * @brief 后退
    * @return 无
    */
    void goBack();

    /**
    * @brief 前进
    * @return 无
    */
    void goForward();

    /**
    * @brief 判断是否可以后退
    * @return 返回 true 表示可以，false 表示不可以
    */
    bool canGoBack();

    /**
    * @brief 判断是否可以前进
    * @return 返回 true 表示可以，false 表示不可以
    */
    bool canGoForward();

    /**
    * @brief 刷新
    * @return 无
    */
    void refresh();

    /**
    * @brief 停止加载
    * @return 无
    */
    void stopLoad();

    /**
    * @brief 是否加载中
    * @return 返回 true 表示加载中，否则为 false
    */
    bool isLoading();

    /**
    * @brief 开始一个下载任务
    * @param[in] url 要下载的文件地址
    * @return 无
    */
    void startDownload(const std::string& url);

    /**
    * @brief 设置页面缩放比例
    * @param[in] zoom_level 比例值
    * @return 无
    */
    void setZoomLevel(float zoom_level);

    /**
    * @brief 获取浏览器对象所属的窗体句柄
    * @return 窗口句柄
    */
    CefWindowHandle getWindowHandle() const;

    /**
    * @brief 获取页面 URL
    * @return 返回 URL 地址
    */
    std::string getURL();

    /**
    * @brief 重设浏览器大小
    * @param[in] width 新的宽度
    * @param[in] height 新的高度
    */
    virtual void resize(int width, int height);

    // /**
    // * @brief 注册一个 ProcessMessageHandler 对象，主要用来处理js消息
    // * @param [in] handler ProcessMessageHandler 对象指针
    // */
    // virtual void registerProcessMessageHandler(ProcessMessageHandler* handler);

    /**
    * @brief 打开开发者工具
    * @return 成功返回 true，失败返回 false
    */
    virtual bool openDevTools();

    /**
    * @brief 关闭开发者工具
    * @return 无
    */
    virtual void closeDevTools();

    /**
    * @brief 判断是否打开开发者工具
    * @return 返回 true 表示已经绑定，false 为未绑定
    */
    virtual bool isDevToolsOpened() const { return devtool_opened_; }

    /**
    * @brief 执行 JavaScript 代码
    * @param[in] script 一段可以执行的 JavaScript 代码
    * @return 无
    */
    virtual void evaluateJavaScript(const std::string& script);

    void onTitleChange(int browserId, const std::string& title);
    void onUrlChange(int browserId, const std::string& oldUrl, const std::string& url);

    void onLoadingStateChange(int browserId, bool isLoading, bool canGoBack, bool canGoForward);
    void onLoadStart(const std::string& url);
    void onLoadEnd(const std::string& url);
    void onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl);

    void onAfterCreated(int browserId);
    void onBeforeClose(int browserId);
protected:
    HWND createSubWindow(HWND parentHwnd, int x, int y,int width, int height, bool showWindow = true);

    void reCreateBrowser();

    void destroy();
private:
    HWND _parentHwnd; // Native window handle for the CefWebView
    HWND _hwnd; // Native window handle for the CefWebView
    std::string _className; // Class name for the CefWebView window
    std::shared_ptr<CefViewClientDelegate> _clientDelegate;
};
}

#endif //!CEFWEBVIEW_H