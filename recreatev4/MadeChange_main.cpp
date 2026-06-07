#include <QApplication>
#include "MadeChanges.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MadeChangesDialog dlg;
    dlg.show();
    return app.exec();
}
