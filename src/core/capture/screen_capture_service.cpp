#include "screen_capture_service.h"

#include "annotation_dialog.h"
#include "region_selection_dialog.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QtMath>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QImageWriter>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>

#include "core/services/log_service.h"

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace iqtools::core {

namespace {

constexpr auto kCaptureLogCategory = "tool.capture";

bool areScalesClose(qreal lhs, qreal rhs)
{
    return qAbs(lhs - rhs) < 0.01;
}

QString formatScale(qreal value)
{
    return QString::number(value, 'f', 2);
}

QString captureStrategyText(bool mixedScale)
{
    return mixedScale
               ? QStringLiteral("mixed_dpi_uniform_density")
               : QStringLiteral("uniform_density");
}

QImage normalizedPixelImage(const QImage& source)
{
    if (source.isNull()) {
        return {};
    }

    QImage normalized = source;
    normalized.setDevicePixelRatio(1.0);
    return normalized;
}

int dotsPerMeterFromDpi(int dpi)
{
    return qRound(static_cast<qreal>(dpi) / 0.0254);
}

QRect pixelRectFromLogicalRect(const QRect& logicalRect, qreal scaleX, qreal scaleY)
{
    return QRect(qRound(logicalRect.x() * scaleX),
                 qRound(logicalRect.y() * scaleY),
                 qMax(1, qRound(logicalRect.width() * scaleX)),
                 qMax(1, qRound(logicalRect.height() * scaleY)));
}

}  // namespace

ScreenCaptureResult ScreenCaptureService::captureAndSave(const CaptureSaveOptions& options) const
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
    if (snapshot.exportImage.isNull()) {
        return {
            false,
            QString(),
            platformCaptureFailureMessage(),
            {},
        };
    }

    QImage imageForSave = snapshot.exportImage;
    if (options.enableAnnotation) {
        AnnotationDialog::Options dialogOptions;
        if (options.annotationOptions != nullptr) {
            dialogOptions.lineWidth = options.annotationOptions->lineWidth;
            dialogOptions.textPixelSize = options.annotationOptions->textPixelSize;
            dialogOptions.mosaicBlockSize = options.annotationOptions->mosaicBlockSize;
            dialogOptions.colorIndex = options.annotationOptions->colorIndex;
            dialogOptions.shortcutRectangle = options.annotationOptions->shortcutRectangle;
            dialogOptions.shortcutArrow = options.annotationOptions->shortcutArrow;
            dialogOptions.shortcutMosaic = options.annotationOptions->shortcutMosaic;
            dialogOptions.shortcutText = options.annotationOptions->shortcutText;
            dialogOptions.shortcutUndo = options.annotationOptions->shortcutUndo;
            dialogOptions.shortcutRedo = options.annotationOptions->shortcutRedo;
            dialogOptions.shortcutColorCycle = options.annotationOptions->shortcutColorCycle;
        }

        AnnotationDialog dialog(imageForSave, dialogOptions, nullptr);
        if (dialog.exec() != QDialog::Accepted) {
            return {
                false,
                QString(),
                QStringLiteral("用户取消标注"),
                {},
            };
        }
        imageForSave = dialog.annotatedImage();

        if (options.annotationOptions != nullptr) {
            const AnnotationDialog::Options acceptedOptions = dialog.options();
            options.annotationOptions->lineWidth = acceptedOptions.lineWidth;
            options.annotationOptions->textPixelSize = acceptedOptions.textPixelSize;
            options.annotationOptions->mosaicBlockSize = acceptedOptions.mosaicBlockSize;
            options.annotationOptions->colorIndex = acceptedOptions.colorIndex;
            options.annotationOptions->shortcutRectangle = acceptedOptions.shortcutRectangle;
            options.annotationOptions->shortcutArrow = acceptedOptions.shortcutArrow;
            options.annotationOptions->shortcutMosaic = acceptedOptions.shortcutMosaic;
            options.annotationOptions->shortcutText = acceptedOptions.shortcutText;
            options.annotationOptions->shortcutUndo = acceptedOptions.shortcutUndo;
            options.annotationOptions->shortcutRedo = acceptedOptions.shortcutRedo;
            options.annotationOptions->shortcutColorCycle = acceptedOptions.shortcutColorCycle;
        }
    }

    return saveImage(imageForSave, options, defaultOutputDirectory());
}

ScreenCaptureResult ScreenCaptureService::captureRegionAndSave(const CaptureSaveOptions& options) const
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
    if (snapshot.previewImage.isNull() || snapshot.exportImage.isNull()) {
        return {
            false,
            QString(),
            platformCaptureFailureMessage(),
            {},
        };
    }

    RegionSelectionDialog selector(QPixmap::fromImage(snapshot.previewImage),
                                   snapshot.logicalGeometry,
                                   nullptr);
    if (selector.exec() != QDialog::Accepted || !selector.hasSelection()) {
        return {
            false,
            QString(),
            QStringLiteral("用户取消截图"),
            {},
        };
    }

    const QRect previewBounds(QPoint(0, 0), snapshot.previewImage.size());
    const QRect selectedLocalRegion =
        selector.selectedRegion().intersected(previewBounds);
    if (!selectedLocalRegion.isValid() || selectedLocalRegion.isEmpty()) {
        return {
            false,
            QString(),
            QStringLiteral("无效选区"),
            {},
        };
    }

    const QRect logicalRegion =
        selectedLocalRegion.translated(snapshot.logicalGeometry.topLeft());

    QImage imageForSave = cropSingleScreenRegion(snapshot, logicalRegion);
    if (imageForSave.isNull()) {
        imageForSave = cropExportRegion(snapshot, logicalRegion);
    }

    if (imageForSave.isNull()) {
        return {
            false,
            QString(),
            QStringLiteral("无效选区"),
            {},
        };
    }

    if (options.enableAnnotation) {
        AnnotationDialog::Options dialogOptions;
        if (options.annotationOptions != nullptr) {
            dialogOptions.lineWidth = options.annotationOptions->lineWidth;
            dialogOptions.textPixelSize = options.annotationOptions->textPixelSize;
            dialogOptions.mosaicBlockSize = options.annotationOptions->mosaicBlockSize;
            dialogOptions.colorIndex = options.annotationOptions->colorIndex;
            dialogOptions.shortcutRectangle = options.annotationOptions->shortcutRectangle;
            dialogOptions.shortcutArrow = options.annotationOptions->shortcutArrow;
            dialogOptions.shortcutMosaic = options.annotationOptions->shortcutMosaic;
            dialogOptions.shortcutText = options.annotationOptions->shortcutText;
            dialogOptions.shortcutUndo = options.annotationOptions->shortcutUndo;
            dialogOptions.shortcutRedo = options.annotationOptions->shortcutRedo;
            dialogOptions.shortcutColorCycle = options.annotationOptions->shortcutColorCycle;
        }

        AnnotationDialog dialog(imageForSave, dialogOptions, nullptr);
        if (dialog.exec() != QDialog::Accepted) {
            return {
                false,
                QString(),
                QStringLiteral("用户取消标注"),
                {},
            };
        }
        imageForSave = dialog.annotatedImage();

        if (options.annotationOptions != nullptr) {
            const AnnotationDialog::Options acceptedOptions = dialog.options();
            options.annotationOptions->lineWidth = acceptedOptions.lineWidth;
            options.annotationOptions->textPixelSize = acceptedOptions.textPixelSize;
            options.annotationOptions->mosaicBlockSize = acceptedOptions.mosaicBlockSize;
            options.annotationOptions->colorIndex = acceptedOptions.colorIndex;
            options.annotationOptions->shortcutRectangle = acceptedOptions.shortcutRectangle;
            options.annotationOptions->shortcutArrow = acceptedOptions.shortcutArrow;
            options.annotationOptions->shortcutMosaic = acceptedOptions.shortcutMosaic;
            options.annotationOptions->shortcutText = acceptedOptions.shortcutText;
            options.annotationOptions->shortcutUndo = acceptedOptions.shortcutUndo;
            options.annotationOptions->shortcutRedo = acceptedOptions.shortcutRedo;
            options.annotationOptions->shortcutColorCycle = acceptedOptions.shortcutColorCycle;
        }
    }

    return saveImage(imageForSave, options, defaultOutputDirectory());
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

    QList<ScreenSlice> slices;
    qreal maxScale = 1.0;
    qreal referenceScale = -1.0;
    bool mixedScale = false;

    for (QScreen* screen : screens) {
        if (screen == nullptr) {
            continue;
        }

        const QPixmap screenshot = screen->grabWindow(0);
        if (screenshot.isNull()) {
            LogService::warning(
                QString::fromLatin1(kCaptureLogCategory),
                QStringLiteral("Screen grab returned null: name=%1 geometry=%2,%3 %4x%5")
                    .arg(screen->name(),
                         QString::number(screen->geometry().x()),
                         QString::number(screen->geometry().y()),
                         QString::number(screen->geometry().width()),
                         QString::number(screen->geometry().height())));
            continue;
        }

        const QImage sourceImage = normalizedPixelImage(screenshot.toImage());
        if (sourceImage.isNull()) {
            LogService::warning(
                QString::fromLatin1(kCaptureLogCategory),
                QStringLiteral("Screen image conversion failed: name=%1")
                    .arg(screen->name()));
            continue;
        }

        const QRect logicalGeometry = screen->geometry();
        if (!logicalGeometry.isValid() || logicalGeometry.isEmpty()) {
            LogService::warning(
                QString::fromLatin1(kCaptureLogCategory),
                QStringLiteral("Screen geometry invalid: name=%1")
                    .arg(screen->name()));
            continue;
        }

        const qreal scaleX =
            static_cast<qreal>(sourceImage.width()) / logicalGeometry.width();
        const qreal scaleY =
            static_cast<qreal>(sourceImage.height()) / logicalGeometry.height();
        const qreal effectiveScale = qMax(scaleX, scaleY);
        if (referenceScale < 0.0) {
            referenceScale = effectiveScale;
        } else if (!areScalesClose(referenceScale, effectiveScale)) {
            mixedScale = true;
        }
        maxScale = qMax(maxScale, effectiveScale);

        ScreenSlice slice;
        slice.name = screen->name().trimmed().isEmpty()
                         ? QStringLiteral("unknown-screen")
                         : screen->name().trimmed();
        slice.logicalGeometry = logicalGeometry;
        slice.sourceImage = sourceImage;
        slice.pixelSize = sourceImage.size();
        slice.screenDevicePixelRatio = screen->devicePixelRatio();
        slice.imageDevicePixelRatio = screenshot.devicePixelRatio();
        slice.scaleX = scaleX;
        slice.scaleY = scaleY;
        slices.push_back(slice);

        LogService::info(
            QString::fromLatin1(kCaptureLogCategory),
            QStringLiteral("Screen slice captured: name=%1 logical=%2,%3 %4x%5 pixels=%6x%7 screenDpr=%8 imageDpr=%9 scaleX=%10 scaleY=%11")
                .arg(slice.name,
                     QString::number(logicalGeometry.x()),
                     QString::number(logicalGeometry.y()),
                     QString::number(logicalGeometry.width()),
                     QString::number(logicalGeometry.height()),
                     QString::number(slice.pixelSize.width()),
                     QString::number(slice.pixelSize.height()),
                     formatScale(slice.screenDevicePixelRatio),
                     formatScale(slice.imageDevicePixelRatio),
                     formatScale(slice.scaleX),
                     formatScale(slice.scaleY)));
    }

    if (slices.isEmpty()) {
        return {};
    }

    const qreal exportScale = qMax(1.0, maxScale);
    QImage previewImage(virtualRect.size(), QImage::Format_ARGB32_Premultiplied);
    previewImage.fill(Qt::transparent);

    const QSize exportSize(qMax(1, qRound(virtualRect.width() * exportScale)),
                           qMax(1, qRound(virtualRect.height() * exportScale)));
    QImage exportImage(exportSize, QImage::Format_ARGB32_Premultiplied);
    exportImage.fill(Qt::transparent);

    QPainter previewPainter(&previewImage);
    previewPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainter exportPainter(&exportImage);
    exportPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    for (const ScreenSlice& slice : slices) {
        const QRect previewTarget(slice.logicalGeometry.topLeft() - virtualRect.topLeft(),
                                  slice.logicalGeometry.size());
        previewPainter.drawImage(previewTarget, slice.sourceImage);

        const QRect exportTarget = scaleLogicalRect(previewTarget, exportScale);
        exportPainter.drawImage(exportTarget, slice.sourceImage);
    }

    LogService::info(
        QString::fromLatin1(kCaptureLogCategory),
        QStringLiteral("Desktop snapshot built: screens=%1 logical=%2,%3 %4x%5 preview=%6x%7 export=%8x%9 exportScale=%10 strategy=%11")
            .arg(QString::number(slices.size()),
                 QString::number(virtualRect.x()),
                 QString::number(virtualRect.y()),
                 QString::number(virtualRect.width()),
                 QString::number(virtualRect.height()),
                 QString::number(previewImage.width()),
                 QString::number(previewImage.height()),
                 QString::number(exportImage.width()),
                 QString::number(exportImage.height()),
                 formatScale(exportScale),
                 captureStrategyText(mixedScale)));

    return {
        virtualRect,
        previewImage,
        exportImage,
        slices,
        exportScale,
        mixedScale,
    };
}

QImage ScreenCaptureService::cropSingleScreenRegion(const DesktopSnapshot& snapshot,
                                                    const QRect& logicalRegion)
{
    if (!logicalRegion.isValid() || logicalRegion.isEmpty()) {
        return {};
    }

    for (const ScreenSlice& slice : snapshot.screens) {
        if (!slice.logicalGeometry.contains(logicalRegion)) {
            continue;
        }

        const QRect relativeLogical =
            logicalRegion.translated(-slice.logicalGeometry.topLeft());
        const QRect pixelRect = pixelRectFromLogicalRect(relativeLogical,
                                                         slice.scaleX,
                                                         slice.scaleY)
                                    .intersected(slice.sourceImage.rect());
        if (!pixelRect.isValid() || pixelRect.isEmpty()) {
            return {};
        }

        LogService::info(
            QString::fromLatin1(kCaptureLogCategory),
            QStringLiteral("Region crop resolved to single screen: name=%1 logical=%2,%3 %4x%5 pixels=%6,%7 %8x%9")
                .arg(slice.name,
                     QString::number(logicalRegion.x()),
                     QString::number(logicalRegion.y()),
                     QString::number(logicalRegion.width()),
                     QString::number(logicalRegion.height()),
                     QString::number(pixelRect.x()),
                     QString::number(pixelRect.y()),
                     QString::number(pixelRect.width()),
                     QString::number(pixelRect.height())));
        return normalizedPixelImage(slice.sourceImage.copy(pixelRect));
    }

    return {};
}

QImage ScreenCaptureService::cropExportRegion(const DesktopSnapshot& snapshot,
                                              const QRect& logicalRegion)
{
    if (snapshot.exportImage.isNull() || !logicalRegion.isValid() || logicalRegion.isEmpty()) {
        return {};
    }

    const QRect localLogicalRegion =
        logicalRegion.translated(-snapshot.logicalGeometry.topLeft());
    const QRect exportRect =
        scaleLogicalRect(localLogicalRegion, snapshot.exportScale)
            .intersected(snapshot.exportImage.rect());
    if (!exportRect.isValid() || exportRect.isEmpty()) {
        return {};
    }

    LogService::info(
        QString::fromLatin1(kCaptureLogCategory),
        QStringLiteral("Region crop fell back to export image: logical=%1,%2 %3x%4 export=%5,%6 %7x%8 strategy=%9")
            .arg(QString::number(logicalRegion.x()),
                 QString::number(logicalRegion.y()),
                 QString::number(logicalRegion.width()),
                 QString::number(logicalRegion.height()),
                 QString::number(exportRect.x()),
                 QString::number(exportRect.y()),
                 QString::number(exportRect.width()),
                 QString::number(exportRect.height()),
                 captureStrategyText(snapshot.mixedScale)));
    return normalizedPixelImage(snapshot.exportImage.copy(exportRect));
}

QRect ScreenCaptureService::scaleLogicalRect(const QRect& logicalRect, qreal scale)
{
    return QRect(qRound(logicalRect.x() * scale),
                 qRound(logicalRect.y() * scale),
                 qMax(1, qRound(logicalRect.width() * scale)),
                 qMax(1, qRound(logicalRect.height() * scale)));
}

ScreenCaptureResult ScreenCaptureService::saveImage(const QImage& image,
                                                    const CaptureSaveOptions& options,
                                                    const QString& defaultDirectory)
{
    ScreenCaptureResult result;

    if (image.isNull()) {
        result.errorMessage = QStringLiteral("截图数据为空");
        return result;
    }

    QString directoryPath = options.outputDirectory.trimmed();
    if (directoryPath.isEmpty()) {
        directoryPath = defaultDirectory;
    }

    QDir directory(directoryPath);
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        result.errorMessage = QStringLiteral("无法创建截图目录: %1").arg(directoryPath);
        return result;
    }

    const QImage scaledImage = scaleImage(image, options.scalePercent);
    if (scaledImage.isNull()) {
        result.errorMessage = QStringLiteral("截图缩放失败");
        return result;
    }

    const QImage finalImage = imageWithDpiMetadata(scaledImage, options.dpi);
    const QString targetFormat = normalizedFormat(options.format);
    const QString fileName = buildFileName(targetFormat);
    const QString filePath = directory.filePath(fileName);

    QImage encodedImage = finalImage;
    if (targetFormat == QStringLiteral("jpg") && encodedImage.hasAlphaChannel()) {
        QImage flattened(encodedImage.size(), QImage::Format_RGB32);
        flattened.fill(Qt::white);
        QPainter painter(&flattened);
        painter.drawImage(0, 0, encodedImage);
        encodedImage = flattened;
    }

    QImageWriter writer(filePath, targetFormat.toUtf8());
    if (targetFormat == QStringLiteral("jpg")) {
        writer.setQuality(qBound(80, options.jpegQuality, 100));
    }

    if (!writer.write(encodedImage)) {
        result.errorMessage =
            QStringLiteral("截图保存失败: %1").arg(writer.errorString());
        return result;
    }

    if (options.copyToClipboard) {
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

QImage ScreenCaptureService::imageWithDpiMetadata(const QImage& source, int dpi)
{
    if (source.isNull()) {
        return {};
    }

    QImage image = source;
    image.setDevicePixelRatio(1.0);
    if (dpi <= 0) {
        image.setDotsPerMeterX(0);
        image.setDotsPerMeterY(0);
        return image;
    }

    const int dotsPerMeter = dotsPerMeterFromDpi(dpi);
    image.setDotsPerMeterX(dotsPerMeter);
    image.setDotsPerMeterY(dotsPerMeter);
    return image;
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
