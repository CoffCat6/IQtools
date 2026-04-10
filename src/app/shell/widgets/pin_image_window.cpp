#include "pin_image_window.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace iqtools::app::widgets {

PinImageWindow::PinImageWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_TranslucentBackground, false);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(6);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(6);

    auto* titleLabel = new QLabel(QStringLiteral("贴图预览"), this);
    titleLabel->setStyleSheet(QStringLiteral("color:#f3f6ff;font-weight:600;"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    auto* copyButton = new QPushButton(QStringLiteral("复制"), this);
    copyButton->setFixedHeight(24);
    headerLayout->addWidget(copyButton);

    auto* saveButton = new QPushButton(QStringLiteral("另存"), this);
    saveButton->setFixedHeight(24);
    headerLayout->addWidget(saveButton);

    auto* closeButton = new QPushButton(QStringLiteral("关闭"), this);
    closeButton->setFixedHeight(24);
    headerLayout->addWidget(closeButton);

    rootLayout->addLayout(headerLayout);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(220, 120);
    m_previewLabel->setStyleSheet(
        QStringLiteral("background:#111827;border:1px solid #4b5563;border-radius:8px;"));
    rootLayout->addWidget(m_previewLabel, 1);

    setStyleSheet(
        QStringLiteral("PinImageWindow{background:#1f2937;border:1px solid #4b5563;border-radius:10px;}"
                       "QPushButton{background:#374151;color:#f9fafb;border:1px solid #4b5563;"
                       "padding:2px 10px;border-radius:6px;}"
                       "QPushButton:hover{background:#4b5563;}"));

    connect(copyButton, &QPushButton::clicked, this, [this]() {
        if (m_image.isNull()) {
            return;
        }
        if (QClipboard* clipboard = QGuiApplication::clipboard(); clipboard != nullptr) {
            clipboard->setImage(m_image);
        }
    });

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        if (m_image.isNull()) {
            return;
        }

        QString defaultDir =
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        if (defaultDir.isEmpty()) {
            defaultDir = QDir::homePath();
        }

        const QString defaultName = QStringLiteral("IQtools_pin_%1.png")
                                        .arg(QDateTime::currentDateTime().toString(
                                            QStringLiteral("yyyyMMdd_HHmmss")));
        const QString selectedPath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("保存贴图"),
            QDir(defaultDir).filePath(defaultName),
            QStringLiteral("PNG 文件 (*.png);;JPEG 文件 (*.jpg *.jpeg)"));
        if (selectedPath.isEmpty()) {
            return;
        }

        m_image.save(selectedPath);
    });

    connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);
}

void PinImageWindow::setImage(const QImage& image)
{
    m_image = image;
    if (m_image.isNull()) {
        m_previewLabel->clear();
        return;
    }

    QSize previewSize = m_image.size();
    previewSize.scale(QSize(880, 520), Qt::KeepAspectRatio);
    if (previewSize.width() < 260) {
        previewSize.setWidth(260);
    }
    if (previewSize.height() < 140) {
        previewSize.setHeight(140);
    }

    resize(previewSize.width() + 16, previewSize.height() + 48);
    updatePreview();
}

void PinImageWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartGlobal = event->globalPosition().toPoint();
        m_dragStartWindowTopLeft = frameGeometry().topLeft();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void PinImageWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        const QPoint delta = event->globalPosition().toPoint() - m_dragStartGlobal;
        move(m_dragStartWindowTopLeft + delta);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void PinImageWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updatePreview();
}

void PinImageWindow::updatePreview()
{
    if (m_previewLabel == nullptr || m_image.isNull()) {
        return;
    }

    const QSize targetSize = m_previewLabel->size();
    const QPixmap pixmap = QPixmap::fromImage(m_image).scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    m_previewLabel->setPixmap(pixmap);
}

}  // namespace iqtools::app::widgets
