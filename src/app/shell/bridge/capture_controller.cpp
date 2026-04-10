#include "capture_controller.h"

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QImage>
#include <QtGui/QShortcut>

#include "app/shell/widgets/pin_image_window.h"
#include "core/capture/i_capture_service.h"
#include "core/services/log_service.h"
#include "core/settings/shortcut_manager.h"
#include "core/settings/settings_manager.h"

namespace iqtools::app::bridge {

namespace {

constexpr auto kCaptureShortcutId = "tool.capture";
constexpr auto kDefaultShortcutRectangle = "1";
constexpr auto kDefaultShortcutArrow = "2";
constexpr auto kDefaultShortcutMosaic = "3";
constexpr auto kDefaultShortcutText = "4";
constexpr auto kDefaultShortcutUndo = "Ctrl+Z";
constexpr auto kDefaultShortcutRedo = "Ctrl+Y";
constexpr auto kDefaultShortcutColorCycle = "C";
constexpr auto kDefaultOutputFormat = "png";
constexpr int kDefaultJpegQuality = 92;
constexpr auto kAnnotationShortcutKeyRectangle = "rectangle";
constexpr auto kAnnotationShortcutKeyArrow = "arrow";
constexpr auto kAnnotationShortcutKeyMosaic = "mosaic";
constexpr auto kAnnotationShortcutKeyText = "text";
constexpr auto kAnnotationShortcutKeyUndo = "undo";
constexpr auto kAnnotationShortcutKeyRedo = "redo";
constexpr auto kAnnotationShortcutKeyColorCycle = "color_cycle";

QString normalizeOutputFormat(const QString& format)
{
    const QString normalized = format.trimmed().toLower();
    if (normalized == QStringLiteral("jpg") ||
        normalized == QStringLiteral("jpeg")) {
        return QStringLiteral("jpg");
    }
    return QStringLiteral("png");
}

int normalizeDpiValue(int dpi)
{
    switch (dpi) {
    case 0:
    case 96:
    case 144:
    case 300:
        return dpi;
    default:
        return 0;
    }
}

}  // namespace

CaptureController::CaptureController(
    iqtools::core::SettingsManager* settingsManager,
    iqtools::core::ShortcutManager* shortcutManager,
    iqtools::core::ICaptureService* captureService,
    QObject* parent)
    : QObject(parent)
    , m_settingsManager(settingsManager)
    , m_shortcutManager(shortcutManager)
    , m_captureService(captureService)
{
    if (m_captureService == nullptr) {
        m_fallbackCaptureService = std::make_unique<iqtools::core::ScreenCaptureService>();
        m_captureService = m_fallbackCaptureService.get();
    }

    m_countdownTimer.setInterval(1000);
    connect(&m_countdownTimer, &QTimer::timeout,
            this, [this]() { onCountdownTick(); });

    m_statusClearTimer.setSingleShot(true);
    connect(&m_statusClearTimer, &QTimer::timeout, this, [this]() {
        if (m_statusMessage.isEmpty()) {
            return;
        }
        m_statusMessage.clear();
        emit statusMessageChanged();
    });

    if (m_shortcutManager != nullptr) {
        connect(m_shortcutManager, &iqtools::core::ShortcutManager::shortcutChanged,
                this, [this](const QString& id, const QKeySequence& key) {
                    onShortcutChanged(id, key);
                });
    }
}

CaptureController::~CaptureController()
{
    if (m_captureShortcutObject != nullptr) {
        m_captureShortcutObject->setEnabled(false);
    }
}

QString CaptureController::statusMessage() const
{
    return m_statusMessage;
}

QString CaptureController::lastCapturePath() const
{
    return m_lastCapturePath;
}

QString CaptureController::outputDirectory() const
{
    if (m_settingsManager == nullptr) {
        return QString();
    }
    return m_settingsManager->captureOutputDirectory();
}

QString CaptureController::defaultOutputDirectory() const
{
    if (captureService() == nullptr) {
        return QString();
    }
    return captureService()->defaultOutputDirectory();
}

bool CaptureController::autoCopyToClipboard() const
{
    if (m_settingsManager == nullptr) {
        return true;
    }
    return m_settingsManager->captureAutoCopyToClipboard();
}

int CaptureController::delaySeconds() const
{
    if (m_settingsManager == nullptr) {
        return 3;
    }
    return m_settingsManager->captureDelaySeconds();
}

int CaptureController::scalePercent() const
{
    if (m_settingsManager == nullptr) {
        return 100;
    }
    return m_settingsManager->captureScalePercent();
}

QString CaptureController::outputFormat() const
{
    if (m_settingsManager == nullptr) {
        return QString::fromLatin1(kDefaultOutputFormat);
    }
    return normalizeOutputFormat(m_settingsManager->captureOutputFormat());
}

int CaptureController::jpegQuality() const
{
    if (m_settingsManager == nullptr) {
        return kDefaultJpegQuality;
    }
    return m_settingsManager->captureJpegQuality();
}

int CaptureController::dpiValue() const
{
    if (m_settingsManager == nullptr) {
        return 0;
    }
    return normalizeDpiValue(m_settingsManager->captureDpi());
}

bool CaptureController::annotationEnabled() const
{
    if (m_settingsManager == nullptr) {
        return false;
    }
    return m_settingsManager->captureAnnotationEnabled();
}

bool CaptureController::pinAfterCapture() const
{
    if (m_settingsManager == nullptr) {
        return false;
    }
    return m_settingsManager->capturePinAfterCapture();
}

int CaptureController::annotationLineWidth() const
{
    if (m_settingsManager == nullptr) {
        return 3;
    }
    return m_settingsManager->captureAnnotationLineWidth();
}

int CaptureController::annotationTextSize() const
{
    if (m_settingsManager == nullptr) {
        return 28;
    }
    return m_settingsManager->captureAnnotationTextSize();
}

int CaptureController::annotationMosaicBlockSize() const
{
    if (m_settingsManager == nullptr) {
        return 10;
    }
    return m_settingsManager->captureAnnotationMosaicBlockSize();
}

int CaptureController::annotationColorIndex() const
{
    if (m_settingsManager == nullptr) {
        return 0;
    }
    return m_settingsManager->captureAnnotationColorIndex();
}

QString CaptureController::annotationShortcutRectangle() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutRectangle),
                                    QString::fromLatin1(kDefaultShortcutRectangle));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutRectangle(),
                                QString::fromLatin1(kDefaultShortcutRectangle));
}

QString CaptureController::annotationShortcutArrow() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutArrow),
                                    QString::fromLatin1(kDefaultShortcutArrow));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutArrow(),
                                QString::fromLatin1(kDefaultShortcutArrow));
}

QString CaptureController::annotationShortcutMosaic() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutMosaic),
                                    QString::fromLatin1(kDefaultShortcutMosaic));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutMosaic(),
                                QString::fromLatin1(kDefaultShortcutMosaic));
}

QString CaptureController::annotationShortcutText() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutText),
                                    QString::fromLatin1(kDefaultShortcutText));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutText(),
                                QString::fromLatin1(kDefaultShortcutText));
}

QString CaptureController::annotationShortcutUndo() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutUndo),
                                    QString::fromLatin1(kDefaultShortcutUndo));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutUndo(),
                                QString::fromLatin1(kDefaultShortcutUndo));
}

QString CaptureController::annotationShortcutRedo() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutRedo),
                                    QString::fromLatin1(kDefaultShortcutRedo));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutRedo(),
                                QString::fromLatin1(kDefaultShortcutRedo));
}

QString CaptureController::annotationShortcutColorCycle() const
{
    if (m_settingsManager == nullptr) {
        return shortcutToNativeText(QString::fromLatin1(kDefaultShortcutColorCycle),
                                    QString::fromLatin1(kDefaultShortcutColorCycle));
    }

    return shortcutToNativeText(m_settingsManager->captureAnnotationShortcutColorCycle(),
                                QString::fromLatin1(kDefaultShortcutColorCycle));
}

QString CaptureController::platformCaptureHint() const
{
    if (captureService() == nullptr) {
        return QString();
    }
    return captureService()->platformCaptureHint();
}

QString CaptureController::captureShortcut() const
{
    if (m_shortcutManager == nullptr) {
        return QKeySequence(QStringLiteral("Alt+C")).toString(QKeySequence::NativeText);
    }

    const QKeySequence key =
        m_shortcutManager->shortcut(QString::fromLatin1(kCaptureShortcutId));
    if (key.isEmpty()) {
        return QString();
    }

    return key.toString(QKeySequence::NativeText);
}

int CaptureController::countdownRemaining() const
{
    return m_countdownRemaining;
}

bool CaptureController::countdownActive() const
{
    return m_countdownTimer.isActive();
}

void CaptureController::setOutputDirectory(const QString& directory)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    QString normalized = QDir::fromNativeSeparators(directory.trimmed());
    if (!normalized.isEmpty()) {
        normalized = QDir::cleanPath(normalized);
    }

    if (m_settingsManager->captureOutputDirectory() == normalized) {
        return;
    }

    m_settingsManager->setCaptureOutputDirectory(normalized);
    emit outputDirectoryChanged();
}

void CaptureController::setAutoCopyToClipboard(bool enabled)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    if (m_settingsManager->captureAutoCopyToClipboard() == enabled) {
        return;
    }

    m_settingsManager->setCaptureAutoCopyToClipboard(enabled);
    emit autoCopyToClipboardChanged();
}

void CaptureController::setDelaySeconds(int seconds)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int bounded = qBound(0, seconds, 10);
    if (m_settingsManager->captureDelaySeconds() == bounded) {
        return;
    }

    m_settingsManager->setCaptureDelaySeconds(bounded);
    emit delaySecondsChanged();
}

void CaptureController::setScalePercent(int percent)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int bounded = qBound(50, percent, 200);
    if (m_settingsManager->captureScalePercent() == bounded) {
        return;
    }

    m_settingsManager->setCaptureScalePercent(bounded);
    emit scalePercentChanged();
}

void CaptureController::setOutputFormat(const QString& format)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeOutputFormat(format);
    if (normalizeOutputFormat(m_settingsManager->captureOutputFormat()) == normalized) {
        return;
    }

    m_settingsManager->setCaptureOutputFormat(normalized);
    emit outputFormatChanged();
}

void CaptureController::setJpegQuality(int quality)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int bounded = qBound(80, quality, 100);
    if (m_settingsManager->captureJpegQuality() == bounded) {
        return;
    }

    m_settingsManager->setCaptureJpegQuality(bounded);
    emit jpegQualityChanged();
}

void CaptureController::setDpiValue(int dpi)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int normalized = normalizeDpiValue(dpi);
    if (normalizeDpiValue(m_settingsManager->captureDpi()) == normalized) {
        return;
    }

    m_settingsManager->setCaptureDpi(normalized);
    emit dpiValueChanged();
}

void CaptureController::setAnnotationEnabled(bool enabled)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    if (m_settingsManager->captureAnnotationEnabled() == enabled) {
        return;
    }

    m_settingsManager->setCaptureAnnotationEnabled(enabled);
    emit annotationEnabledChanged();
}

void CaptureController::setPinAfterCapture(bool enabled)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    if (m_settingsManager->capturePinAfterCapture() == enabled) {
        return;
    }

    m_settingsManager->setCapturePinAfterCapture(enabled);
    emit pinAfterCaptureChanged();
}

void CaptureController::setAnnotationLineWidth(int width)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int normalized = qBound(1, width, 12);
    if (m_settingsManager->captureAnnotationLineWidth() == normalized) {
        return;
    }

    m_settingsManager->setCaptureAnnotationLineWidth(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationTextSize(int size)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int normalized = qBound(12, size, 96);
    if (m_settingsManager->captureAnnotationTextSize() == normalized) {
        return;
    }

    m_settingsManager->setCaptureAnnotationTextSize(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationMosaicBlockSize(int size)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int normalized = qBound(4, size, 64);
    if (m_settingsManager->captureAnnotationMosaicBlockSize() == normalized) {
        return;
    }

    m_settingsManager->setCaptureAnnotationMosaicBlockSize(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationColorIndex(int index)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const int normalized = qBound(0, index, 7);
    if (m_settingsManager->captureAnnotationColorIndex() == normalized) {
        return;
    }

    m_settingsManager->setCaptureAnnotationColorIndex(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutRectangle(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutRectangle));
    if (m_settingsManager->captureAnnotationShortcutRectangle() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyRectangle),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutRectangle(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutArrow(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutArrow));
    if (m_settingsManager->captureAnnotationShortcutArrow() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyArrow),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutArrow(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutMosaic(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutMosaic));
    if (m_settingsManager->captureAnnotationShortcutMosaic() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyMosaic),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutMosaic(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutText(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutText));
    if (m_settingsManager->captureAnnotationShortcutText() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyText),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutText(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutUndo(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutUndo));
    if (m_settingsManager->captureAnnotationShortcutUndo() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyUndo),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutUndo(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutRedo(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutRedo));
    if (m_settingsManager->captureAnnotationShortcutRedo() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyRedo),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutRedo(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::setAnnotationShortcutColorCycle(const QString& text)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    const QString normalized = normalizeShortcutInput(text,
                                                      QString::fromLatin1(kDefaultShortcutColorCycle));
    if (m_settingsManager->captureAnnotationShortcutColorCycle() == normalized) {
        return;
    }

    QString conflictKey;
    if (hasAnnotationShortcutConflict(normalized,
                                      QString::fromLatin1(kAnnotationShortcutKeyColorCycle),
                                      &conflictKey)) {
        setStatusMessage(
            tr("标注快捷键冲突：%1 已被 %2 使用")
                .arg(shortcutToNativeText(normalized, normalized),
                     annotationShortcutActionName(conflictKey)),
            4500);
        emit annotationOptionsChanged();
        return;
    }

    m_settingsManager->setCaptureAnnotationShortcutColorCycle(normalized);
    emit annotationOptionsChanged();
}

void CaptureController::attachToWindow(QObject* windowObject)
{
    if (windowObject == nullptr) {
        return;
    }

    if (m_hostWindow == windowObject && m_captureShortcutObject != nullptr) {
        return;
    }

    m_hostWindow = windowObject;

    if (m_captureShortcutObject != nullptr) {
        m_captureShortcutObject->deleteLater();
        m_captureShortcutObject = nullptr;
    }

    m_captureShortcutObject = new QShortcut(m_hostWindow);
    m_captureShortcutObject->setContext(Qt::ApplicationShortcut);
    connect(m_captureShortcutObject, &QShortcut::activated,
            this, &CaptureController::captureNow);

    updateShortcutBinding();
}

void CaptureController::captureNow()
{
    performCapture(CaptureMode::FullScreen);
}

void CaptureController::captureRegion()
{
    performCapture(CaptureMode::Region);
}

void CaptureController::captureWithDelay(int seconds)
{
    startCaptureCountdown(CaptureMode::FullScreen, seconds);
}

void CaptureController::captureRegionWithDelay(int seconds)
{
    startCaptureCountdown(CaptureMode::Region, seconds);
}

void CaptureController::cancelDelayedCapture()
{
    if (!countdownActive()) {
        return;
    }

    m_countdownTimer.stop();
    m_countdownRemaining = 0;
    emit countdownChanged();
    setStatusMessage(tr("已取消延时截图"), 2500);
}

void CaptureController::openOutputDirectory() const
{
    const QString directory = resolvedOutputDirectory();
    QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
}

void CaptureController::useDefaultOutputDirectory()
{
    if (m_settingsManager == nullptr) {
        return;
    }

    if (m_settingsManager->captureOutputDirectory().isEmpty()) {
        return;
    }

    m_settingsManager->setCaptureOutputDirectory(QString());
    emit outputDirectoryChanged();
}

void CaptureController::pinLastCapture()
{
    if (m_lastCapturePath.trimmed().isEmpty()) {
        setStatusMessage(tr("暂无可贴图的截图"), 2500);
        return;
    }

    if (showPinnedImageFromFile(m_lastCapturePath)) {
        setStatusMessage(tr("已打开贴图窗口"), 2500);
    } else {
        setStatusMessage(tr("贴图失败：无法读取截图文件"), 3000);
    }
}

void CaptureController::setAnnotationShortcutByKey(const QString& key, const QString& text)
{
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRectangle)) {
        setAnnotationShortcutRectangle(text);
        return;
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyArrow)) {
        setAnnotationShortcutArrow(text);
        return;
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyMosaic)) {
        setAnnotationShortcutMosaic(text);
        return;
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyText)) {
        setAnnotationShortcutText(text);
        return;
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyUndo)) {
        setAnnotationShortcutUndo(text);
        return;
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRedo)) {
        setAnnotationShortcutRedo(text);
        return;
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyColorCycle)) {
        setAnnotationShortcutColorCycle(text);
    }
}

QString CaptureController::annotationShortcutByKey(const QString& key) const
{
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRectangle)) {
        return annotationShortcutRectangle();
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyArrow)) {
        return annotationShortcutArrow();
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyMosaic)) {
        return annotationShortcutMosaic();
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyText)) {
        return annotationShortcutText();
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyUndo)) {
        return annotationShortcutUndo();
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRedo)) {
        return annotationShortcutRedo();
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyColorCycle)) {
        return annotationShortcutColorCycle();
    }
    return QString();
}

QString CaptureController::annotationShortcutConflictAction(const QString& shortcutText,
                                                           const QString& selfKey) const
{
    const QString normalized = normalizeShortcutInput(shortcutText, QString());
    if (normalized.trimmed().isEmpty()) {
        return QString();
    }

    QString conflict;
    if (hasAnnotationShortcutConflict(normalized, selfKey.trimmed(), &conflict)) {
        return conflict;
    }
    return QString();
}

QString CaptureController::annotationShortcutActionLabel(const QString& key) const
{
    return annotationShortcutActionName(key);
}

QString CaptureController::shortcutPortableFromKeyEvent(int key, int modifiers) const
{
    if (isModifierOnlyKey(key)) {
        return QString();
    }

    const QKeySequence sequence(static_cast<int>(modifiers) | key);
    const QString portable = sequence.toString(QKeySequence::PortableText).trimmed();
    if (portable.isEmpty()) {
        return QString();
    }

    return portable;
}

QString CaptureController::shortcutDisplayText(const QString& portableText) const
{
    return shortcutToNativeText(portableText, portableText);
}

bool CaptureController::isModifierOnlyKey(int key) const
{
    switch (key) {
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
        return true;
    default:
        return false;
    }
}

void CaptureController::onCountdownTick()
{
    if (m_countdownRemaining <= 1) {
        m_countdownTimer.stop();
        m_countdownRemaining = 0;
        emit countdownChanged();
        performCapture(m_pendingMode);
        return;
    }

    --m_countdownRemaining;
    emit countdownChanged();

    const QString modeText =
        (m_pendingMode == CaptureMode::Region) ? tr("区域截图") : tr("全屏截图");
    setStatusMessage(tr("%1 秒后执行%2").arg(m_countdownRemaining).arg(modeText));
}

void CaptureController::onShortcutChanged(const QString& id, const QKeySequence& newKey)
{
    Q_UNUSED(newKey);
    if (id != QString::fromLatin1(kCaptureShortcutId)) {
        return;
    }

    updateShortcutBinding();
    emit captureShortcutChanged();
}

void CaptureController::updateShortcutBinding()
{
    if (m_captureShortcutObject == nullptr) {
        return;
    }

    QKeySequence key = QKeySequence(QStringLiteral("Alt+C"));
    if (m_shortcutManager != nullptr) {
        const QKeySequence configured =
            m_shortcutManager->shortcut(QString::fromLatin1(kCaptureShortcutId));
        if (!configured.isEmpty()) {
            key = configured;
        }
    }

    m_captureShortcutObject->setKey(key);
    emit captureShortcutChanged();
}

void CaptureController::setStatusMessage(const QString& message, int timeoutMs)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();

    if (timeoutMs <= 0) {
        m_statusClearTimer.stop();
        return;
    }

    m_statusClearTimer.start(timeoutMs);
}

QString CaptureController::resolvedOutputDirectory() const
{
    const QString customDirectory = outputDirectory().trimmed();
    if (customDirectory.isEmpty()) {
        return defaultOutputDirectory();
    }
    return QDir::toNativeSeparators(customDirectory);
}

iqtools::core::CaptureSaveOptions CaptureController::captureSaveOptions(
    iqtools::core::AnnotationOptions* annotationOptions) const
{
    iqtools::core::CaptureSaveOptions options;
    options.outputDirectory = resolvedOutputDirectory();
    options.format = outputFormat();
    options.copyToClipboard = autoCopyToClipboard();
    options.scalePercent = scalePercent();
    options.jpegQuality = jpegQuality();
    options.dpi = dpiValue();
    options.enableAnnotation = annotationEnabled();
    options.annotationOptions = annotationOptions;
    return options;
}

iqtools::core::AnnotationOptions CaptureController::annotationOptions() const
{
    iqtools::core::AnnotationOptions options;
    if (m_settingsManager == nullptr) {
        options.shortcutRectangle = QString::fromLatin1(kDefaultShortcutRectangle);
        options.shortcutArrow = QString::fromLatin1(kDefaultShortcutArrow);
        options.shortcutMosaic = QString::fromLatin1(kDefaultShortcutMosaic);
        options.shortcutText = QString::fromLatin1(kDefaultShortcutText);
        options.shortcutUndo = QString::fromLatin1(kDefaultShortcutUndo);
        options.shortcutRedo = QString::fromLatin1(kDefaultShortcutRedo);
        options.shortcutColorCycle = QString::fromLatin1(kDefaultShortcutColorCycle);
        return options;
    }

    options.lineWidth = m_settingsManager->captureAnnotationLineWidth();
    options.textPixelSize = m_settingsManager->captureAnnotationTextSize();
    options.mosaicBlockSize = m_settingsManager->captureAnnotationMosaicBlockSize();
    options.colorIndex = m_settingsManager->captureAnnotationColorIndex();
    options.shortcutRectangle = m_settingsManager->captureAnnotationShortcutRectangle();
    options.shortcutArrow = m_settingsManager->captureAnnotationShortcutArrow();
    options.shortcutMosaic = m_settingsManager->captureAnnotationShortcutMosaic();
    options.shortcutText = m_settingsManager->captureAnnotationShortcutText();
    options.shortcutUndo = m_settingsManager->captureAnnotationShortcutUndo();
    options.shortcutRedo = m_settingsManager->captureAnnotationShortcutRedo();
    options.shortcutColorCycle = m_settingsManager->captureAnnotationShortcutColorCycle();
    return options;
}

void CaptureController::persistAnnotationOptions(const iqtools::core::AnnotationOptions& options)
{
    if (m_settingsManager == nullptr) {
        return;
    }

    bool changed = false;

    const int lineWidth = qBound(1, options.lineWidth, 12);
    if (m_settingsManager->captureAnnotationLineWidth() != lineWidth) {
        m_settingsManager->setCaptureAnnotationLineWidth(lineWidth);
        changed = true;
    }

    const int textSize = qBound(12, options.textPixelSize, 96);
    if (m_settingsManager->captureAnnotationTextSize() != textSize) {
        m_settingsManager->setCaptureAnnotationTextSize(textSize);
        changed = true;
    }

    const int mosaicSize = qBound(4, options.mosaicBlockSize, 64);
    if (m_settingsManager->captureAnnotationMosaicBlockSize() != mosaicSize) {
        m_settingsManager->setCaptureAnnotationMosaicBlockSize(mosaicSize);
        changed = true;
    }

    const int colorIndex = qBound(0, options.colorIndex, 7);
    if (m_settingsManager->captureAnnotationColorIndex() != colorIndex) {
        m_settingsManager->setCaptureAnnotationColorIndex(colorIndex);
        changed = true;
    }

    const QString rectangleShortcut = normalizeShortcutInput(
        options.shortcutRectangle,
        QString::fromLatin1(kDefaultShortcutRectangle));
    if (m_settingsManager->captureAnnotationShortcutRectangle() != rectangleShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutRectangle(rectangleShortcut);
        changed = true;
    }

    const QString arrowShortcut = normalizeShortcutInput(
        options.shortcutArrow,
        QString::fromLatin1(kDefaultShortcutArrow));
    if (m_settingsManager->captureAnnotationShortcutArrow() != arrowShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutArrow(arrowShortcut);
        changed = true;
    }

    const QString mosaicShortcut = normalizeShortcutInput(
        options.shortcutMosaic,
        QString::fromLatin1(kDefaultShortcutMosaic));
    if (m_settingsManager->captureAnnotationShortcutMosaic() != mosaicShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutMosaic(mosaicShortcut);
        changed = true;
    }

    const QString textShortcut = normalizeShortcutInput(
        options.shortcutText,
        QString::fromLatin1(kDefaultShortcutText));
    if (m_settingsManager->captureAnnotationShortcutText() != textShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutText(textShortcut);
        changed = true;
    }

    const QString undoShortcut = normalizeShortcutInput(
        options.shortcutUndo,
        QString::fromLatin1(kDefaultShortcutUndo));
    if (m_settingsManager->captureAnnotationShortcutUndo() != undoShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutUndo(undoShortcut);
        changed = true;
    }

    const QString redoShortcut = normalizeShortcutInput(
        options.shortcutRedo,
        QString::fromLatin1(kDefaultShortcutRedo));
    if (m_settingsManager->captureAnnotationShortcutRedo() != redoShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutRedo(redoShortcut);
        changed = true;
    }

    const QString colorShortcut = normalizeShortcutInput(
        options.shortcutColorCycle,
        QString::fromLatin1(kDefaultShortcutColorCycle));
    if (m_settingsManager->captureAnnotationShortcutColorCycle() != colorShortcut) {
        m_settingsManager->setCaptureAnnotationShortcutColorCycle(colorShortcut);
        changed = true;
    }

    if (changed) {
        emit annotationOptionsChanged();
    }
}

QString CaptureController::shortcutToNativeText(const QString& portableText,
                                                const QString& fallbackPortable) const
{
    const QString source = portableText.trimmed().isEmpty()
                               ? fallbackPortable
                               : portableText.trimmed();
    QKeySequence shortcut = QKeySequence::fromString(source, QKeySequence::PortableText);
    if (shortcut.isEmpty()) {
        shortcut = QKeySequence::fromString(source, QKeySequence::NativeText);
    }
    if (shortcut.isEmpty()) {
        return source;
    }
    return shortcut.toString(QKeySequence::NativeText);
}

QString CaptureController::normalizeShortcutInput(const QString& inputText,
                                                  const QString& fallbackPortable) const
{
    const QString source = inputText.trimmed().isEmpty()
                               ? fallbackPortable
                               : inputText.trimmed();

    QKeySequence shortcut = QKeySequence::fromString(source, QKeySequence::PortableText);
    if (shortcut.isEmpty()) {
        shortcut = QKeySequence::fromString(source, QKeySequence::NativeText);
    }
    if (shortcut.isEmpty()) {
        shortcut = QKeySequence::fromString(fallbackPortable, QKeySequence::PortableText);
    }
    return shortcut.toString(QKeySequence::PortableText);
}

QString CaptureController::annotationShortcutValueForKey(const QString& key) const
{
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRectangle)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutRectangle);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutRectangle(),
                                      QString::fromLatin1(kDefaultShortcutRectangle));
    }

    if (key == QString::fromLatin1(kAnnotationShortcutKeyArrow)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutArrow);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutArrow(),
                                      QString::fromLatin1(kDefaultShortcutArrow));
    }

    if (key == QString::fromLatin1(kAnnotationShortcutKeyMosaic)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutMosaic);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutMosaic(),
                                      QString::fromLatin1(kDefaultShortcutMosaic));
    }

    if (key == QString::fromLatin1(kAnnotationShortcutKeyText)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutText);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutText(),
                                      QString::fromLatin1(kDefaultShortcutText));
    }

    if (key == QString::fromLatin1(kAnnotationShortcutKeyUndo)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutUndo);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutUndo(),
                                      QString::fromLatin1(kDefaultShortcutUndo));
    }

    if (key == QString::fromLatin1(kAnnotationShortcutKeyRedo)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutRedo);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutRedo(),
                                      QString::fromLatin1(kDefaultShortcutRedo));
    }

    if (key == QString::fromLatin1(kAnnotationShortcutKeyColorCycle)) {
        if (m_settingsManager == nullptr) {
            return QString::fromLatin1(kDefaultShortcutColorCycle);
        }
        return normalizeShortcutInput(m_settingsManager->captureAnnotationShortcutColorCycle(),
                                      QString::fromLatin1(kDefaultShortcutColorCycle));
    }

    return QString();
}

QString CaptureController::annotationShortcutActionName(const QString& key) const
{
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRectangle)) {
        return tr("矩形工具");
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyArrow)) {
        return tr("箭头工具");
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyMosaic)) {
        return tr("马赛克工具");
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyText)) {
        return tr("文字工具");
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyUndo)) {
        return tr("撤销");
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyRedo)) {
        return tr("重做");
    }
    if (key == QString::fromLatin1(kAnnotationShortcutKeyColorCycle)) {
        return tr("换色");
    }
    return tr("其他动作");
}

bool CaptureController::hasAnnotationShortcutConflict(const QString& candidatePortable,
                                                      const QString& selfKey,
                                                      QString* conflictKey) const
{
    const QString normalizedCandidate = candidatePortable.trimmed();
    if (normalizedCandidate.isEmpty()) {
        return false;
    }

    const char* keys[] = {
        kAnnotationShortcutKeyRectangle,
        kAnnotationShortcutKeyArrow,
        kAnnotationShortcutKeyMosaic,
        kAnnotationShortcutKeyText,
        kAnnotationShortcutKeyUndo,
        kAnnotationShortcutKeyRedo,
        kAnnotationShortcutKeyColorCycle,
    };

    for (const char* keyRaw : keys) {
        const QString key = QString::fromLatin1(keyRaw);
        if (key == selfKey) {
            continue;
        }

        const QString currentValue = annotationShortcutValueForKey(key);
        if (currentValue == normalizedCandidate) {
            if (conflictKey != nullptr) {
                *conflictKey = key;
            }
            return true;
        }
    }

    return false;
}

void CaptureController::performCapture(CaptureMode mode)
{
    if (countdownActive()) {
        m_countdownTimer.stop();
        m_countdownRemaining = 0;
        emit countdownChanged();
    }

    iqtools::core::ScreenCaptureResult result;
    iqtools::core::AnnotationOptions options = annotationOptions();
    iqtools::core::AnnotationOptions* optionsPtr = annotationEnabled() ? &options : nullptr;
    const iqtools::core::CaptureSaveOptions saveOptions = captureSaveOptions(optionsPtr);
    iqtools::core::ICaptureService* service = captureService();
    if (service == nullptr) {
        setStatusMessage(tr("截图服务不可用"), 5000);
        emit captureFailed(tr("截图服务不可用"));
        return;
    }

    const QString modeText =
        (mode == CaptureMode::Region) ? QStringLiteral("region") : QStringLiteral("fullscreen");
    iqtools::core::LogService::info(
        QStringLiteral("tool.capture"),
        QStringLiteral("Capture requested: mode=%1 format=%2 scale=%3 jpegQuality=%4 dpi=%5 copyToClipboard=%6 annotation=%7")
            .arg(modeText,
                 saveOptions.format,
                 QString::number(saveOptions.scalePercent),
                 QString::number(saveOptions.jpegQuality),
                 QString::number(saveOptions.dpi),
                 saveOptions.copyToClipboard ? QStringLiteral("true")
                                             : QStringLiteral("false"),
                 saveOptions.enableAnnotation ? QStringLiteral("true")
                                              : QStringLiteral("false")));

    if (mode == CaptureMode::Region) {
        result = service->captureRegionAndSave(saveOptions);
    } else {
        result = service->captureAndSave(saveOptions);
    }

    if (!result.success) {
        if (result.errorMessage == QStringLiteral("用户取消截图") ||
            result.errorMessage == QStringLiteral("用户取消标注")) {
            setStatusMessage(tr("已取消截图"), 2000);
            return;
        }

        const QString error = tr("截图失败：%1").arg(result.errorMessage);
        setStatusMessage(error, 5000);
        emit captureFailed(result.errorMessage);
        iqtools::core::LogService::warning(
            QStringLiteral("tool.capture"),
            QStringLiteral("Capture failed: %1").arg(result.errorMessage));
        return;
    }

    m_lastCapturePath = result.filePath;
    emit lastCapturePathChanged();
    emit captureCompleted(m_lastCapturePath);

    if (annotationEnabled()) {
        persistAnnotationOptions(options);
    }

    const bool pinned = pinAfterCapture() && showPinnedImageFromFile(m_lastCapturePath);

    const QString status = autoCopyToClipboard()
                               ? tr("截图已保存并复制到剪贴板")
                               : tr("截图已保存");
    const QString finalStatus = pinned ? status + tr("，已贴图") : status;
    setStatusMessage(finalStatus, 4500);

    iqtools::core::LogService::info(
        QStringLiteral("tool.capture"),
        QStringLiteral("Capture completed: %1 (%2x%3) format=%4 scale=%5 jpegQuality=%6 dpi=%7")
            .arg(m_lastCapturePath)
            .arg(result.imageSize.width())
            .arg(result.imageSize.height())
            .arg(saveOptions.format)
            .arg(saveOptions.scalePercent)
            .arg(saveOptions.jpegQuality)
            .arg(saveOptions.dpi));
}

void CaptureController::startCaptureCountdown(CaptureMode mode, int seconds)
{
    const int effectiveSeconds =
        (seconds < 0) ? delaySeconds() : qBound(0, seconds, 10);
    if (effectiveSeconds <= 0) {
        performCapture(mode);
        return;
    }

    m_pendingMode = mode;
    if (m_countdownTimer.isActive()) {
        m_countdownTimer.stop();
    }
    m_countdownRemaining = effectiveSeconds;
    emit countdownChanged();

    const QString modeText =
        (mode == CaptureMode::Region) ? tr("区域截图") : tr("全屏截图");
    setStatusMessage(tr("%1 秒后执行%2").arg(m_countdownRemaining).arg(modeText));
    m_countdownTimer.start();
}

bool CaptureController::showPinnedImageFromFile(const QString& filePath)
{
    const QString normalizedPath = filePath.trimmed();
    if (normalizedPath.isEmpty()) {
        return false;
    }

    QImage image(normalizedPath);
    if (image.isNull()) {
        return false;
    }

    if (m_pinWindow == nullptr) {
        m_pinWindow = std::make_unique<iqtools::app::widgets::PinImageWindow>();
    }

    m_pinWindow->setImage(image);
    m_pinWindow->show();
    m_pinWindow->raise();
    m_pinWindow->activateWindow();
    return true;
}

iqtools::core::ICaptureService* CaptureController::captureService() const
{
    return m_captureService;
}

}  // namespace iqtools::app::bridge
