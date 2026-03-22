#include "main_window.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "core/services/log_service.h"

namespace iqtools::app {

MainWindow::MainWindow(iqtools::core::AppContext* appContext, QWidget* parent)
    : QMainWindow(parent)
    , m_appContext(appContext)
{
    setupUi();
    iqtools::core::LogService::info(QStringLiteral("app.shell"), QStringLiteral("Main window created"));
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("IQtools"));
    resize(1200, 760);

    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Navigation panel
    auto* navWidget = new QWidget(central);
    navWidget->setFixedWidth(220);
    navWidget->setStyleSheet(QStringLiteral("QWidget { background-color: #2b2b2b; }"));
    auto* navLayout = new QVBoxLayout(navWidget);
    navLayout->setContentsMargins(0, 20, 0, 0);
    navLayout->setSpacing(4);

    // App title
    auto* titleLabel = new QLabel(QStringLiteral("IQtools"), navWidget);
    titleLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #ffffff; font-size: 18px; font-weight: bold; padding: 10px 20px; }"));
    navLayout->addWidget(titleLabel);
    navLayout->addSpacing(20);

    // Navigation items
    const QStringList navItems = {
        QStringLiteral("首页"),
        QStringLiteral("截图工具"),
        QStringLiteral("翻译工具"),
        QStringLiteral("插件管理"),
        QStringLiteral("设置")
    };

    for (int i = 0; i < navItems.size(); ++i) {
        auto* btn = new QPushButton(navItems[i], navWidget);
        btn->setFixedHeight(44);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: transparent; color: #cccccc; border: none; "
            "text-align: left; padding-left: 20px; font-size: 14px; border-radius: 4px; } "
            "QPushButton:hover { background-color: #3b3b3b; color: #ffffff; } "
            "QPushButton:pressed { background-color: #4b4b4b; color: #ffffff; }"));
        connect(btn, &QPushButton::clicked, this, [this, i]() { onNavigationClicked(i); });
        navLayout->addWidget(btn);
    }

    navLayout->addStretch();

    // Content area
    auto* contentWidget = new QWidget(central);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(30, 30, 30, 30);

    m_contentLabel = new QLabel(
        QStringLiteral("<h1 style='color:#333;'>IQtools</h1>"
                      "<p style='color:#666;font-size:14px;'>项目骨架已初始化。</p>"
                      "<p style='color:#999;font-size:12px;'>请从左侧导航选择工具开始使用。</p>"),
        contentWidget);
    m_contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto* versionLabel = new QLabel(QStringLiteral("v0.1.0"), contentWidget);
    versionLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #999; font-size: 12px; padding-top: 10px; }"));

    contentLayout->addWidget(m_contentLabel);
    contentLayout->addWidget(versionLabel);
    contentLayout->addStretch();

    rootLayout->addWidget(navWidget);
    rootLayout->addWidget(contentWidget, 1);

    setCentralWidget(central);
}

void MainWindow::onNavigationClicked(int index)
{
    const QStringList contentTexts = {
        QStringLiteral("<h1 style='color:#333;'>首页</h1>"
                       "<p style='color:#666;font-size:14px;'>欢迎使用 IQtools 工具箱。</p>"),
        QStringLiteral("<h1 style='color:#333;'>截图工具</h1>"
                       "<p style='color:#666;font-size:14px;'>截图工具页面（待实现）。</p>"),
        QStringLiteral("<h1 style='color:#333;'>翻译工具</h1>"
                       "<p style='color:#666;font-size:14px;'>翻译工具页面（待实现）。</p>"),
        QStringLiteral("<h1 style='color:#333;'>插件管理</h1>"
                       "<p style='color:#666;font-size:14px;'>插件管理页面（待实现）。</p>"),
        QStringLiteral("<h1 style='color:#333;'>设置</h1>"
                       "<p style='color:#666;font-size:14px;'>设置页面（待实现）。</p>")
    };

    if (index >= 0 && index < contentTexts.size()) {
        m_contentLabel->setText(contentTexts[index]);
        iqtools::core::LogService::info(QStringLiteral("app.shell"),
            QStringLiteral("Navigation: %1").arg(index));
    }
}

}  // namespace iqtools::app
