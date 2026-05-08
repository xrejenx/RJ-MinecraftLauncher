#include <QString>
#include <QMessageBox>

namespace FetchHelper {
    void handleFetchError(const QString &error) {
        QMessageBox::warning(nullptr, "Update Error", 
            "Failed to check for updates. The launcher will continue offline.\nError: " + error);
    }
}