#pragma once

#include <QtCore/QString>

namespace iqtools::core {

struct AnnotationOptions;
struct ScreenCaptureResult;

class ICaptureService {
public:
    virtual ~ICaptureService() = default;

    virtual ScreenCaptureResult captureAndSave(const QString& outputDirectory,
                                               const QString& format,
                                               bool copyToClipboard,
                                               int scalePercent,
                                               bool enableAnnotation,
                                               AnnotationOptions* annotationOptions) const = 0;

    virtual ScreenCaptureResult captureRegionAndSave(const QString& outputDirectory,
                                                     const QString& format,
                                                     bool copyToClipboard,
                                                     int scalePercent,
                                                     bool enableAnnotation,
                                                     AnnotationOptions* annotationOptions) const = 0;

    virtual QString defaultOutputDirectory() const = 0;
    virtual QString platformCaptureHint() const = 0;
};

}  // namespace iqtools::core
