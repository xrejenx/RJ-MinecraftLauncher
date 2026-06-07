#ifndef RJML_WEBVIEW_WRAPPER_H
#define RJML_WEBVIEW_WRAPPER_H

#include <QWidget>

namespace RJMLWebView {
    void initialize();
    QWidget* createWebView(QWidget* parent);
}

#endif // RJML_WEBVIEW_WRAPPER_H