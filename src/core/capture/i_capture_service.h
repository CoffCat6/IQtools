#pragma once

#include <QtCore/QSize>
#include <QtCore/QString>

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

struct CaptureSaveOptions {
    QString outputDirectory;
    QString format {QStringLiteral("png")};
    bool copyToClipboard {true};
    int scalePercent {100};
    int jpegQuality {92};
    int dpi {0};
    bool enableAnnotation {false};
    AnnotationOptions* annotationOptions {nullptr};
};

class ICaptureService {
public:
    virtual ~ICaptureService() = default;

    virtual ScreenCaptureResult captureAndSave(const CaptureSaveOptions& options) const = 0;

    virtual ScreenCaptureResult captureRegionAndSave(const CaptureSaveOptions& options) const = 0;

    virtual QString defaultOutputDirectory() const = 0;
    virtual QString platformCaptureHint() const = 0;
};

}  // namespace iqtools::core
