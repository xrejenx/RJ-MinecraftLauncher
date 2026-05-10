#ifndef RJML_WEBVIEW_WRAPPER_H
#define RJML_WEBVIEW_WRAPPER_H

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <objbase.h>
// Placeholder for WebView2 specific includes
// #include <WebView2.h>
#else
#include <QtWebView/QtWebView>
#endif

namespace RJMLWebView {
    inline void initialize() {
#ifdef Q_OS_WIN
        // Initialize COM for native Windows WebView2 (MinGW compatible)
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#else
        QtWebView::initialize();
#endif
    }
}

#endif // RJML_WEBVIEW_WRAPPER_H