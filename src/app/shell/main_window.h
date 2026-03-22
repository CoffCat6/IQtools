#pragma once

#include <QtWidgets/QMainWindow>

class QListWidget;
class QLabel;

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
    QListWidget* m_navigationList {nullptr};
    QLabel* m_contentLabel {nullptr};
};

}  // namespace iqtools::app
