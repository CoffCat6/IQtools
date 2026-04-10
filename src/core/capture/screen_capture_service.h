#pragma once

#include <QtCore/QList>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtGui/QImage>

#include "core/capture/i_capture_service.h"

namespace iqtools::core {

class ScreenCaptureService : public ICaptureService {
public:
    ScreenCaptureResult captureAndSave(const CaptureSaveOptions& options) const override;

    ScreenCaptureResult captureRegionAndSave(const CaptureSaveOptions& options) const override;

    QString defaultOutputDirectory() const override;
    QString platformCaptureHint() const override;

private:
    struct ScreenSlice {
        QString name;
        QRect logicalGeometry;
        QImage sourceImage;
        QSize pixelSize;
        qreal screenDevicePixelRatio {1.0};
        qreal imageDevicePixelRatio {1.0};
        qreal scaleX {1.0};
        qreal scaleY {1.0};
    };

    struct DesktopSnapshot {
        QRect logicalGeometry;
        QImage previewImage;
        QImage exportImage;
        QList<ScreenSlice> screens;
        qreal exportScale {1.0};
        bool mixedScale {false};
    };

    static DesktopSnapshot captureVirtualDesktop();
    static QImage cropSingleScreenRegion(const DesktopSnapshot& snapshot,
                                         const QRect& logicalRegion);
    static QImage cropExportRegion(const DesktopSnapshot& snapshot,
                                   const QRect& logicalRegion);
    static QRect scaleLogicalRect(const QRect& logicalRect, qreal scale);
    static ScreenCaptureResult saveImage(const QImage& image,
                                         const CaptureSaveOptions& options,
                                         const QString& defaultDirectory);
    static QImage scaleImage(const QImage& source, int scalePercent);
    static QImage imageWithDpiMetadata(const QImage& source, int dpi);
    static QString normalizedFormat(const QString& format);
    static QString buildFileName(const QString& format);
    static bool isWaylandSession();
    static bool hasMacScreenCapturePermission();
    static QString platformCaptureFailureMessage();
};

}  // namespace iqtools::core
