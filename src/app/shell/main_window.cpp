#include "main_window.h"

#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "core/services/log_service.h"
#include "core/services/theme_manager.h"

namespace iqtools::app {

namespace {

QFrame* createBentoCard(const QString& title,
                        const QString& text,
                        const QString& meta,
                        const QString& variant,
                        QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("BentoCard"));
    if (!variant.isEmpty()) {
        card->setProperty("variant", variant);
    }

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(10);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("CardTitle"));

    auto* textLabel = new QLabel(text, card);
    textLabel->setObjectName(QStringLiteral("CardText"));
    textLabel->setWordWrap(true);

    auto* metaLabel = new QLabel(meta, card);
    metaLabel->setObjectName(QStringLiteral("CardMeta"));

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(textLabel, 1);
    cardLayout->addWidget(metaLabel);

    return card;
}

QWidget* createPlaceholderPage(const QString& title, const QString& desc, QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    auto* card = createBentoCard(title, desc, QStringLiteral("Coming soon"), QString(), page);
    layout->addWidget(card);
    layout->addStretch();

    return page;
}

QWidget* createHomePage(QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(page);
    scrollArea->setObjectName(QStringLiteral("MainScroll"));
    scrollArea->setWidgetResizable(true);

    auto* gridHost = new QWidget(scrollArea);
    gridHost->setObjectName(QStringLiteral("BentoGrid"));

    auto* grid = new QGridLayout(gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(16);

    auto* cardA = createBentoCard(
        QStringLiteral("工具总览"),
        QStringLiteral("集中管理截图、翻译与插件化工具，保持清晰的模块边界。"),
        QStringLiteral("Overview"),
        QStringLiteral("highlight"),
        gridHost);

    auto* cardB = createBentoCard(
        QStringLiteral("截图工具"),
        QStringLiteral("区域截图 / 全屏截图 / 结果处理链（OCR、翻译、导出）预留。"),
        QStringLiteral("Capture"),
        QString(),
        gridHost);

    auto* cardC = createBentoCard(
        QStringLiteral("翻译工具"),
        QStringLiteral("统一翻译流程 UI，后续可接入多个 provider。"),
        QStringLiteral("Translate"),
        QStringLiteral("accent"),
        gridHost);

    auto* cardD = createBentoCard(
        QStringLiteral("插件系统"),
        QStringLiteral("Qt 原生插件机制，先稳接口，再扩展生态。"),
        QStringLiteral("Plugin"),
        QString(),
        gridHost);

    auto* cardE = createBentoCard(
        QStringLiteral("主题系统"),
        QStringLiteral("内置浅色/深色主题，后续支持自定义 QSS 扩展。"),
        QStringLiteral("Theme"),
        QString(),
        gridHost);

    auto* cardF = createBentoCard(
        QStringLiteral("日志系统"),
        QStringLiteral("统一日志入口，支持控制台与文件输出。"),
        QStringLiteral("Logging"),
        QString(),
        gridHost);

    grid->addWidget(cardA, 0, 0, 2, 2);
    grid->addWidget(cardB, 0, 2, 1, 2);
    grid->addWidget(cardC, 1, 2, 1, 1);
    grid->addWidget(cardD, 1, 3, 1, 1);
    grid->addWidget(cardE, 2, 0, 1, 2);
    grid->addWidget(cardF, 2, 2, 1, 2);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);

    scrollArea->setWidget(gridHost);
    pageLayout->addWidget(scrollArea);
    return page;
}

QWidget* createSettingsPage(QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    auto* card = createBentoCard(
        QStringLiteral("主题设置"),
        QStringLiteral("选择默认主题（浅色/深色）。后续可扩展自定义主题。"),
        QStringLiteral("Theme settings"),
        QStringLiteral("highlight"),
        page);

    auto* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    if (cardLayout != nullptr) {
        auto* row = new QWidget(card);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        auto* lightBtn = new QPushButton(QStringLiteral("浅色主题"), row);
        lightBtn->setObjectName(QStringLiteral("NavButton"));
        auto* darkBtn = new QPushButton(QStringLiteral("深色主题"), row);
        darkBtn->setObjectName(QStringLiteral("NavButton"));

        rowLayout->addWidget(lightBtn);
        rowLayout->addWidget(darkBtn);
        rowLayout->addStretch();

        QObject::connect(lightBtn, &QPushButton::clicked, row, []() {
            auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
            iqtools::core::ThemeManager::applyTheme(app, iqtools::core::AppTheme::Light);
        });

        QObject::connect(darkBtn, &QPushButton::clicked, row, []() {
            auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
            iqtools::core::ThemeManager::applyTheme(app, iqtools::core::AppTheme::Dark);
        });

        cardLayout->addWidget(row);
    }

    layout->addWidget(card);
    layout->addStretch();
    return page;
}

}  // namespace

MainWindow::MainWindow(iqtools::core::AppContext* appContext, QWidget* parent)
    : QMainWindow(parent)
    , m_appContext(appContext)
{
    setObjectName(QStringLiteral("MainWindow"));
    setupUi();
    iqtools::core::LogService::info(QStringLiteral("app.shell"), QStringLiteral("Main window created"));
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("IQtools"));
    resize(1280, 820);

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("CentralRoot"));

    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* navPanel = new QWidget(central);
    navPanel->setObjectName(QStringLiteral("NavPanel"));
    navPanel->setFixedWidth(240);

    auto* navLayout = new QVBoxLayout(navPanel);
    navLayout->setContentsMargins(18, 18, 18, 18);
    navLayout->setSpacing(8);

    auto* navTitle = new QLabel(QStringLiteral("IQtools"), navPanel);
    navTitle->setObjectName(QStringLiteral("NavTitle"));
    navLayout->addWidget(navTitle);
    navLayout->addSpacing(8);

    const QStringList navItems = {
        QStringLiteral("首页"),
        QStringLiteral("截图工具"),
        QStringLiteral("翻译工具"),
        QStringLiteral("插件管理"),
        QStringLiteral("设置")
    };

    for (int i = 0; i < navItems.size(); ++i) {
        auto* btn = new QPushButton(navItems[i], navPanel);
        btn->setObjectName(QStringLiteral("NavButton"));
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, i]() { onNavigationClicked(i); });
        m_navigationButtons.append(btn);
        navLayout->addWidget(btn);
    }

    if (!m_navigationButtons.isEmpty()) {
        m_navigationButtons.first()->setChecked(true);
    }

    navLayout->addStretch();

    auto* contentPanel = new QWidget(central);
    contentPanel->setObjectName(QStringLiteral("ContentPanel"));

    auto* contentLayout = new QVBoxLayout(contentPanel);
    contentLayout->setContentsMargins(20, 20, 20, 20);

    m_pageStack = new QStackedWidget(contentPanel);
    m_pageStack->addWidget(createHomePage(m_pageStack));
    m_pageStack->addWidget(createPlaceholderPage(QStringLiteral("截图工具"),
                                                 QStringLiteral("截图工具页面（待实现）。"),
                                                 m_pageStack));
    m_pageStack->addWidget(createPlaceholderPage(QStringLiteral("翻译工具"),
                                                 QStringLiteral("翻译工具页面（待实现）。"),
                                                 m_pageStack));
    m_pageStack->addWidget(createPlaceholderPage(QStringLiteral("插件管理"),
                                                 QStringLiteral("插件管理页面（待实现）。"),
                                                 m_pageStack));
    m_pageStack->addWidget(createSettingsPage(m_pageStack));

    contentLayout->addWidget(m_pageStack);

    rootLayout->addWidget(navPanel);
    rootLayout->addWidget(contentPanel, 1);

    setCentralWidget(central);
}

void MainWindow::onNavigationClicked(int index)
{
    for (int i = 0; i < m_navigationButtons.size(); ++i) {
        m_navigationButtons[i]->setChecked(i == index);
    }

    if (m_pageStack != nullptr && index >= 0 && index < m_pageStack->count()) {
        m_pageStack->setCurrentIndex(index);
        iqtools::core::LogService::info(QStringLiteral("app.shell"),
                                        QStringLiteral("Navigation switched: %1").arg(index));
    }
}

}  // namespace iqtools::app
