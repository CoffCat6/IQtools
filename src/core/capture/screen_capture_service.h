#pragma once

#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtGui/QImage>

#include "core/capture/i_capture_service.h"

namespace iqtools::core {

struct AnnotationOptions {
    int lineWidth {3};
    int textPixelSize {28};
    int mosaicBlockSize {10};
    int colorIndex {0};

    QString shortcutRectangle;
    QString shortcutArrow;
    QString shortcutMosaic;
    QString shortcutText;
    QString shortcutUndo;
    QString shortcutRedo;
    QString shortcutColorCycle;
};

struct ScreenCaptureResult {
    bool success {false};
    QString filePath;
    QString errorMessage;
    QSize imageSize;
};

class ScreenCaptureService : public ICaptureService {
public:
    ScreenCaptureResult captureAndSave(const QString& outputDirectory,
                                       const QString& format,
                                       bool copyToClipboard,
                                       int scalePercent = 100,
                                       bool enableAnnotation = false,
                                       AnnotationOptions* annotationOptions = nullptr) const override;

    ScreenCaptureResult captureRegionAndSave(const QString& outputDirectory,
                                             const QString& format,
                                             bool copyToClipboard,
                                             int scalePercent = 100,
                                             bool enableAnnotation = false,
                                             AnnotationOptions* annotationOptions = nullptr) const override;

    QString defaultOutputDirectory() const override;
    QString platformCaptureHint() const override;

private:
    struct DesktopSnapshot {
        QImage image;
        QRect geometry;
    };

    static DesktopSnapshot captureVirtualDesktop();
    static ScreenCaptureResult saveImage(const QImage& image,
                                         const QString& outputDirectory,
                                         const QString& format,
                                         bool copyToClipboard,
                                         int scalePercent,
                                         const QString& defaultDirectory);
    static QImage scaleImage(const QImage& source, int scalePercent);
    static QString normalizedFormat(const QString& format);
    static QString buildFileName(const QString& format);
    static bool isWaylandSession();
    static bool hasMacScreenCapturePermission();
    static QString platformCaptureFailureMessage();
};

}  // namespace iqtools::core
