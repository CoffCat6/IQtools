#pragma once

#include <QtCore/QList>
#include <QtWidgets/QMainWindow>

class QLabel;
class QPushButton;
class QStackedWidget;

namespace iqtools::core {
class AppContext;
}

namespace iqtools::app {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(iqtools::core::AppContext* appContext, QWidget* parent = nullptr);

private:
    void setupUi();

private:
    void onNavigationClicked(int index);

private:
    iqtools::core::AppContext* m_appContext {nullptr};
    QList<QPushButton*> m_navigationButtons;
    QStackedWidget* m_pageStack {nullptr};
};

}  // namespace iqtools::app
