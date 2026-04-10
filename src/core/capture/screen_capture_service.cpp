#include "screen_capture_service.h"

#include "annotation_dialog.h"
#include "region_selection_dialog.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace iqtools::core {

ScreenCaptureResult ScreenCaptureService::captureAndSave(
    const QString& outputDirectory,
    const QString& format,
    bool copyToClipboard,
    int scalePercent,
    bool enableAnnotation,
    AnnotationOptions* annotationOptions) const
{
#if defined(Q_OS_MACOS)
    if (!hasMacScreenCapturePermission()) {
        return {
            false,
            QString(),
            platformCaptureFailureMessage(),
            {},
        };
    }
#endif

    const DesktopSnapshot snapshot = captureVirtualDesktop();
    if (snapshot.image.isNull()) {
        return {
            false,
            QString(),
            platformCaptureFailureMessage(),
            {},
        };
    }

    QImage imageForSave = snapshot.image;
    if (enableAnnotation) {
        AnnotationDialog::Options dialogOptions;
        if (annotationOptions != nullptr) {
            dialogOptions.lineWidth = annotationOptions->lineWidth;
            dialogOptions.textPixelSize = annotationOptions->textPixelSize;
            dialogOptions.mosaicBlockSize = annotationOptions->mosaicBlockSize;
            dialogOptions.colorIndex = annotationOptions->colorIndex;
            dialogOptions.shortcutRectangle = annotationOptions->shortcutRectangle;
            dialogOptions.shortcutArrow = annotationOptions->shortcutArrow;
            dialogOptions.shortcutMosaic = annotationOptions->shortcutMosaic;
            dialogOptions.shortcutText = annotationOptions->shortcutText;
            dialogOptions.shortcutUndo = annotationOptions->shortcutUndo;
            dialogOptions.shortcutRedo = annotationOptions->shortcutRedo;
            dialogOptions.shortcutColorCycle = annotationOptions->shortcutColorCycle;
        }

        AnnotationDialog dialog(imageForSave, dialogOptions, nullptr);
        dialog.show();
        if (dialog.exec() != QDialog::Accepted) {
            return {
                false,
                QString(),
                QStringLiteral("用户取消标注"),
                {},
            };
        }
        imageForSave = dialog.annotatedImage();

        if (annotationOptions != nullptr) {
            const AnnotationDialog::Options acceptedOptions = dialog.options();
            annotationOptions->lineWidth = acceptedOptions.lineWidth;
            annotationOptions->textPixelSize = acceptedOptions.textPixelSize;
            annotationOptions->mosaicBlockSize = acceptedOptions.mosaicBlockSize;
            annotationOptions->colorIndex = acceptedOptions.colorIndex;
            annotationOptions->shortcutRectangle = acceptedOptions.shortcutRectangle;
            annotationOptions->shortcutArrow = acceptedOptions.shortcutArrow;
            annotationOptions->shortcutMosaic = acceptedOptions.shortcutMosaic;
            annotationOptions->shortcutText = acceptedOptions.shortcutText;
            annotationOptions->shortcutUndo = acceptedOptions.shortcutUndo;
            annotationOptions->shortcutRedo = acceptedOptions.shortcutRedo;
            annotationOptions->shortcutColorCycle = acceptedOptions.shortcutColorCycle;
        }
    }

    return saveImage(imageForSave,
                     outputDirectory,
                     format,
                     copyToClipboard,
                     scalePercent,
                     defaultOutputDirectory());
}

ScreenCaptureResult ScreenCaptureService::captureRegionAndSave(
    const QString& outputDirectory,
    const QString& format,
    bool copyToClipboard,
    int scalePercent,
    bool enableAnnotation,
    AnnotationOptions* annotationOptions) const
{
#if defined(Q_OS_MACOS)
    if (!hasMacScreenCapturePermission()) {
        return {
            false,
            QString(),
            platformCaptureFailureMessage(),
            {},
        };
    }
#endif

    const DesktopSnapshot snapshot = captureVirtualDesktop();
    if (snapshot.image.isNull()) {
        return {
            false,
            QString(),
            platformCaptureFailureMessage(),
            {},
        };
    }

    RegionSelectionDialog selector(QPixmap::fromImage(snapshot.image),
                                   snapshot.geometry,
                                   nullptr);
    selector.show();

    if (selector.exec() != QDialog::Accepted || !selector.hasSelection()) {
        return {
            false,
            QString(),
            QStringLiteral("用户取消截图"),
            {},
        };
    }

    const QRect region = selector.selectedRegion().intersected(snapshot.image.rect());
    if (!region.isValid() || region.isEmpty()) {
        return {
            false,
            QString(),
            QStringLiteral("无效选区"),
            {},
        };
    }

    QImage imageForSave = snapshot.image.copy(region);
    if (enableAnnotation) {
        AnnotationDialog::Options dialogOptions;
        if (annotationOptions != nullptr) {
            dialogOptions.lineWidth = annotationOptions->lineWidth;
            dialogOptions.textPixelSize = annotationOptions->textPixelSize;
            dialogOptions.mosaicBlockSize = annotationOptions->mosaicBlockSize;
            dialogOptions.colorIndex = annotationOptions->colorIndex;
            dialogOptions.shortcutRectangle = annotationOptions->shortcutRectangle;
            dialogOptions.shortcutArrow = annotationOptions->shortcutArrow;
            dialogOptions.shortcutMosaic = annotationOptions->shortcutMosaic;
            dialogOptions.shortcutText = annotationOptions->shortcutText;
            dialogOptions.shortcutUndo = annotationOptions->shortcutUndo;
            dialogOptions.shortcutRedo = annotationOptions->shortcutRedo;
            dialogOptions.shortcutColorCycle = annotationOptions->shortcutColorCycle;
        }

        AnnotationDialog dialog(imageForSave, dialogOptions, nullptr);
        dialog.show();
        if (dialog.exec() != QDialog::Accepted) {
            return {
                false,
                QString(),
                QStringLiteral("用户取消标注"),
                {},
            };
        }
        imageForSave = dialog.annotatedImage();

        if (annotationOptions != nullptr) {
            const AnnotationDialog::Options acceptedOptions = dialog.options();
            annotationOptions->lineWidth = acceptedOptions.lineWidth;
            annotationOptions->textPixelSize = acceptedOptions.textPixelSize;
            annotationOptions->mosaicBlockSize = acceptedOptions.mosaicBlockSize;
            annotationOptions->colorIndex = acceptedOptions.colorIndex;
            annotationOptions->shortcutRectangle = acceptedOptions.shortcutRectangle;
            annotationOptions->shortcutArrow = acceptedOptions.shortcutArrow;
            annotationOptions->shortcutMosaic = acceptedOptions.shortcutMosaic;
            annotationOptions->shortcutText = acceptedOptions.shortcutText;
            annotationOptions->shortcutUndo = acceptedOptions.shortcutUndo;
            annotationOptions->shortcutRedo = acceptedOptions.shortcutRedo;
            annotationOptions->shortcutColorCycle = acceptedOptions.shortcutColorCycle;
        }
    }

    return saveImage(imageForSave,
                     outputDirectory,
                     format,
                     copyToClipboard,
                     scalePercent,
                     defaultOutputDirectory());
}

QString ScreenCaptureService::defaultOutputDirectory() const
{
    QString picturesDirectory =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picturesDirectory.isEmpty()) {
        picturesDirectory =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    QDir directory(picturesDirectory);
    return QDir::toNativeSeparators(
        directory.filePath(QStringLiteral("IQtoolsCaptures")));
}

QString ScreenCaptureService::platformCaptureHint() const
{
#if defined(Q_OS_MACOS)
    if (!hasMacScreenCapturePermission()) {
        return QStringLiteral("macOS 首次截图前需在“系统设置-隐私与安全性-屏幕录制”中授权 IQtools。");
    }
#endif

#if defined(Q_OS_LINUX)
    if (isWaylandSession()) {
        return QStringLiteral("检测到 Wayland 会话：若无法截图，请确认 xdg-desktop-portal 与 PipeWire 可用，或切换到 X11。当前版本将自动走兼容降级提示。");
    }
#endif

    return QString();
}

ScreenCaptureService::DesktopSnapshot ScreenCaptureService::captureVirtualDesktop()
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return {};
    }

    QRect virtualRect;
    for (QScreen* screen : screens) {
        if (screen == nullptr) {
            continue;
        }
        virtualRect = virtualRect.united(screen->geometry());
    }

    if (!virtualRect.isValid() || virtualRect.isEmpty()) {
        return {};
    }

    QImage stitchedImage(virtualRect.size(), QImage::Format_ARGB32_Premultiplied);
    stitchedImage.fill(Qt::transparent);

    QPainter painter(&stitchedImage);
    for (QScreen* screen : screens) {
        if (screen == nullptr) {
            continue;
        }

        const QPixmap snapshot = screen->grabWindow(0);
        if (snapshot.isNull()) {
            continue;
        }

        const QPoint targetTopLeft =
            screen->geometry().topLeft() - virtualRect.topLeft();
        painter.drawPixmap(targetTopLeft, snapshot);
    }

    return {stitchedImage, virtualRect};
}

ScreenCaptureResult ScreenCaptureService::saveImage(const QImage& image,
                                                    const QString& outputDirectory,
                                                    const QString& format,
                                                    bool copyToClipboard,
                                                    int scalePercent,
                                                    const QString& defaultDirectory)
{
    ScreenCaptureResult result;

    if (image.isNull()) {
        result.errorMessage = QStringLiteral("截图数据为空");
        return result;
    }

    QString directoryPath = outputDirectory.trimmed();
    if (directoryPath.isEmpty()) {
        directoryPath = defaultDirectory;
    }

    QDir directory(directoryPath);
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        result.errorMessage = QStringLiteral("无法创建截图目录: %1").arg(directoryPath);
        return result;
    }

    const QImage finalImage = scaleImage(image, scalePercent);
    if (finalImage.isNull()) {
        result.errorMessage = QStringLiteral("截图缩放失败");
        return result;
    }

    const QString targetFormat = normalizedFormat(format);
    const QString fileName = buildFileName(targetFormat);
    const QString filePath = directory.filePath(fileName);
    const QByteArray formatUpper = targetFormat.toUpper().toUtf8();
    if (!finalImage.save(filePath, formatUpper.constData())) {
        result.errorMessage = QStringLiteral("截图保存失败");
        return result;
    }

    if (copyToClipboard) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        if (clipboard != nullptr) {
            clipboard->setImage(finalImage);
        }
    }

    result.success = true;
    result.filePath = QDir::toNativeSeparators(filePath);
    result.imageSize = finalImage.size();
    return result;
}

QImage ScreenCaptureService::scaleImage(const QImage& source, int scalePercent)
{
    if (source.isNull()) {
        return {};
    }

    const int boundedPercent = qBound(50, scalePercent, 200);
    if (boundedPercent == 100) {
        return source;
    }

    const QSize targetSize = QSize(
        qMax(1, source.width() * boundedPercent / 100),
        qMax(1, source.height() * boundedPercent / 100));
    return source.scaled(targetSize,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation);
}

QString ScreenCaptureService::normalizedFormat(const QString& format)
{
    const QString normalized = format.trimmed().toLower();
    if (normalized == QStringLiteral("jpg") ||
        normalized == QStringLiteral("jpeg")) {
        return QStringLiteral("jpg");
    }
    return QStringLiteral("png");
}

QString ScreenCaptureService::buildFileName(const QString& format)
{
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    return QStringLiteral("IQtools_capture_%1.%2").arg(timestamp, format);
}

bool ScreenCaptureService::isWaylandSession()
{
#if defined(Q_OS_LINUX)
    const QString platformName = QGuiApplication::platformName().trimmed().toLower();
    if (platformName.contains(QStringLiteral("wayland"))) {
        return true;
    }

    const QString sessionType =
        QString::fromLocal8Bit(qgetenv("XDG_SESSION_TYPE")).trimmed().toLower();
    return sessionType == QStringLiteral("wayland");
#else
    return false;
#endif
}

bool ScreenCaptureService::hasMacScreenCapturePermission()
{
#if defined(Q_OS_MACOS)
    return CGPreflightScreenCaptureAccess();
#else
    return true;
#endif
}

QString ScreenCaptureService::platformCaptureFailureMessage()
{
#if defined(Q_OS_MACOS)
    if (!hasMacScreenCapturePermission()) {
        return QStringLiteral("缺少 macOS 屏幕录制权限，请前往“系统设置-隐私与安全性-屏幕录制”为 IQtools 授权后重试。");
    }
#endif

#if defined(Q_OS_LINUX)
    if (isWaylandSession()) {
        return QStringLiteral("Wayland 会话下截图失败：请确认 xdg-desktop-portal 与 PipeWire 服务已启动，或切换到 X11 会话后重试。");
    }
#endif

    return QStringLiteral("未能获取屏幕图像");
}

}  // namespace iqtools::core
