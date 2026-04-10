#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtGui/QKeySequence>

#include "core/capture/screen_capture_service.h"

class QShortcut;

namespace iqtools::core {
class SettingsManager;
class ShortcutManager;
class ICaptureService;
class ScreenCaptureService;
}

namespace iqtools::app::widgets {
class PinImageWindow;
}

namespace iqtools::app::bridge {

class CaptureController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastCapturePath READ lastCapturePath NOTIFY lastCapturePathChanged)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY outputDirectoryChanged)
    Q_PROPERTY(QString defaultOutputDirectory READ defaultOutputDirectory CONSTANT)
    Q_PROPERTY(bool autoCopyToClipboard READ autoCopyToClipboard WRITE setAutoCopyToClipboard NOTIFY autoCopyToClipboardChanged)
    Q_PROPERTY(int delaySeconds READ delaySeconds WRITE setDelaySeconds NOTIFY delaySecondsChanged)
    Q_PROPERTY(int scalePercent READ scalePercent WRITE setScalePercent NOTIFY scalePercentChanged)
    Q_PROPERTY(bool annotationEnabled READ annotationEnabled WRITE setAnnotationEnabled NOTIFY annotationEnabledChanged)
    Q_PROPERTY(bool pinAfterCapture READ pinAfterCapture WRITE setPinAfterCapture NOTIFY pinAfterCaptureChanged)
    Q_PROPERTY(int annotationLineWidth READ annotationLineWidth WRITE setAnnotationLineWidth NOTIFY annotationOptionsChanged)
    Q_PROPERTY(int annotationTextSize READ annotationTextSize WRITE setAnnotationTextSize NOTIFY annotationOptionsChanged)
    Q_PROPERTY(int annotationMosaicBlockSize READ annotationMosaicBlockSize WRITE setAnnotationMosaicBlockSize NOTIFY annotationOptionsChanged)
    Q_PROPERTY(int annotationColorIndex READ annotationColorIndex WRITE setAnnotationColorIndex NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutRectangle READ annotationShortcutRectangle WRITE setAnnotationShortcutRectangle NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutArrow READ annotationShortcutArrow WRITE setAnnotationShortcutArrow NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutMosaic READ annotationShortcutMosaic WRITE setAnnotationShortcutMosaic NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutText READ annotationShortcutText WRITE setAnnotationShortcutText NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutUndo READ annotationShortcutUndo WRITE setAnnotationShortcutUndo NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutRedo READ annotationShortcutRedo WRITE setAnnotationShortcutRedo NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString annotationShortcutColorCycle READ annotationShortcutColorCycle WRITE setAnnotationShortcutColorCycle NOTIFY annotationOptionsChanged)
    Q_PROPERTY(QString platformCaptureHint READ platformCaptureHint CONSTANT)
    Q_PROPERTY(QString captureShortcut READ captureShortcut NOTIFY captureShortcutChanged)
    Q_PROPERTY(int countdownRemaining READ countdownRemaining NOTIFY countdownChanged)
    Q_PROPERTY(bool countdownActive READ countdownActive NOTIFY countdownChanged)

public:
    explicit CaptureController(iqtools::core::SettingsManager* settingsManager,
                               iqtools::core::ShortcutManager* shortcutManager,
                               iqtools::core::ICaptureService* captureService = nullptr,
                               QObject* parent = nullptr);
    ~CaptureController() override;

    QString statusMessage() const;
    QString lastCapturePath() const;
    QString outputDirectory() const;
    QString defaultOutputDirectory() const;
    bool autoCopyToClipboard() const;
    int delaySeconds() const;
    int scalePercent() const;
    bool annotationEnabled() const;
    bool pinAfterCapture() const;
    int annotationLineWidth() const;
    int annotationTextSize() const;
    int annotationMosaicBlockSize() const;
    int annotationColorIndex() const;
    QString annotationShortcutRectangle() const;
    QString annotationShortcutArrow() const;
    QString annotationShortcutMosaic() const;
    QString annotationShortcutText() const;
    QString annotationShortcutUndo() const;
    QString annotationShortcutRedo() const;
    QString annotationShortcutColorCycle() const;
    QString platformCaptureHint() const;
    QString captureShortcut() const;
    int countdownRemaining() const;
    bool countdownActive() const;

    void setOutputDirectory(const QString& directory);
    void setAutoCopyToClipboard(bool enabled);
    void setDelaySeconds(int seconds);
    void setScalePercent(int percent);
    void setAnnotationEnabled(bool enabled);
    void setPinAfterCapture(bool enabled);
    void setAnnotationLineWidth(int width);
    void setAnnotationTextSize(int size);
    void setAnnotationMosaicBlockSize(int size);
    void setAnnotationColorIndex(int index);
    void setAnnotationShortcutRectangle(const QString& text);
    void setAnnotationShortcutArrow(const QString& text);
    void setAnnotationShortcutMosaic(const QString& text);
    void setAnnotationShortcutText(const QString& text);
    void setAnnotationShortcutUndo(const QString& text);
    void setAnnotationShortcutRedo(const QString& text);
    void setAnnotationShortcutColorCycle(const QString& text);

    Q_INVOKABLE void attachToWindow(QObject* windowObject);
    Q_INVOKABLE void captureNow();
    Q_INVOKABLE void captureRegion();
    Q_INVOKABLE void captureWithDelay(int seconds = -1);
    Q_INVOKABLE void captureRegionWithDelay(int seconds = -1);
    Q_INVOKABLE void cancelDelayedCapture();
    Q_INVOKABLE void openOutputDirectory() const;
    Q_INVOKABLE void useDefaultOutputDirectory();
    Q_INVOKABLE void pinLastCapture();
    Q_INVOKABLE void setAnnotationShortcutByKey(const QString& key, const QString& text);
    Q_INVOKABLE QString annotationShortcutByKey(const QString& key) const;
    Q_INVOKABLE QString annotationShortcutConflictAction(const QString& shortcutText,
                                                         const QString& selfKey) const;
    Q_INVOKABLE QString annotationShortcutActionLabel(const QString& key) const;
    Q_INVOKABLE QString shortcutPortableFromKeyEvent(int key, int modifiers) const;
    Q_INVOKABLE QString shortcutDisplayText(const QString& portableText) const;
    Q_INVOKABLE bool isModifierOnlyKey(int key) const;

signals:
    void statusMessageChanged();
    void lastCapturePathChanged();
    void outputDirectoryChanged();
    void autoCopyToClipboardChanged();
    void delaySecondsChanged();
    void scalePercentChanged();
    void annotationEnabledChanged();
    void pinAfterCaptureChanged();
    void annotationOptionsChanged();
    void captureShortcutChanged();
    void countdownChanged();
    void captureCompleted(const QString& filePath);
    void captureFailed(const QString& errorMessage);

private:
    enum class CaptureMode {
        FullScreen,
        Region,
    };

    void performCapture(CaptureMode mode);
    void startCaptureCountdown(CaptureMode mode, int seconds);
    void onCountdownTick();
    void onShortcutChanged(const QString& id, const QKeySequence& newKey);
    void updateShortcutBinding();
    void setStatusMessage(const QString& message, int timeoutMs = 0);
    QString resolvedOutputDirectory() const;
    iqtools::core::AnnotationOptions annotationOptions() const;
    void persistAnnotationOptions(const iqtools::core::AnnotationOptions& options);
    QString shortcutToNativeText(const QString& portableText,
                                 const QString& fallbackPortable) const;
    QString normalizeShortcutInput(const QString& inputText,
                                   const QString& fallbackPortable) const;
    QString annotationShortcutValueForKey(const QString& key) const;
    QString annotationShortcutActionName(const QString& key) const;
    bool hasAnnotationShortcutConflict(const QString& candidatePortable,
                                       const QString& selfKey,
                                       QString* conflictKey) const;
    bool showPinnedImageFromFile(const QString& filePath);
    iqtools::core::ICaptureService* captureService() const;

private:
    iqtools::core::SettingsManager* m_settingsManager {nullptr};
    iqtools::core::ShortcutManager* m_shortcutManager {nullptr};
    iqtools::core::ICaptureService* m_captureService {nullptr};
    std::unique_ptr<iqtools::core::ScreenCaptureService> m_fallbackCaptureService;
    std::unique_ptr<iqtools::app::widgets::PinImageWindow> m_pinWindow;

    QObject* m_hostWindow {nullptr};
    QShortcut* m_captureShortcutObject {nullptr};
    QTimer m_countdownTimer;
    QTimer m_statusClearTimer;

    QString m_statusMessage;
    QString m_lastCapturePath;
    int m_countdownRemaining {0};
    CaptureMode m_pendingMode {CaptureMode::FullScreen};
};

}  // namespace iqtools::app::bridge
