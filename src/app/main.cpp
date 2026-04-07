#include <QtWidgets/QApplication>
#include <QtQuickControls2/QQuickStyle>

#include "app/bootstrap/application_bootstrap.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QApplication::setApplicationName(QStringLiteral("IQtools"));
    QApplication::setOrganizationName(QStringLiteral("IQtools"));

    iqtools::app::ApplicationBootstrap bootstrap;
    bootstrap.initialize();

    return app.exec();
}
