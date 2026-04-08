#include "update_controller.h"

#include <QtCore/QTimer>
#include <QtGui/QDesktopServices>

#include "core/services/update_checker.h"

namespace iqtools::app::bridge {

UpdateController::UpdateController(iqtools::core::UpdateChecker* checker, QObject* parent)
    : QObject(parent)
    , m_checker(checker)
{
    connect(m_checker, &iqtools::core::UpdateChecker::checkingChanged,
            this, &UpdateController::checkingChanged);

    connect(m_checker, &iqtools::core::UpdateChecker::updateCheckFinished,
            this, &UpdateController::onUpdateCheckFinished);

    connect(m_checker, &iqtools::core::UpdateChecker::updateCheckFailed,
            this, &UpdateController::onUpdateCheckFailed);
}

QString UpdateController::currentVersion() const {
    return iqtools::core::UpdateChecker::currentVersion();
}

QString UpdateController::buildDate() const {
    return iqtools::core::UpdateChecker::buildDate();
}

bool UpdateController::checking() const {
    return m_checker->isChecking();
}

bool UpdateController::hasUpdate() const {
    return m_hasUpdate;
}

QString UpdateController::latestVersion() const {
    return m_latestVersion;
}

QString UpdateController::releaseNotes() const {
    return m_releaseNotes;
}

QString UpdateController::releaseUrl() const {
    return m_releaseUrl;
}

QString UpdateController::statusMessage() const {
    return m_statusMessage;
}

void UpdateController::checkNow() {
    m_statusMessage = QStringLiteral("正在检查更新...");
    emit statusMessageChanged();
    m_checker->checkForUpdates();
}

void UpdateController::openReleasePage() {
    if (!m_releaseUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_releaseUrl));
    }
}

void UpdateController::openDownloadPage() {
    if (!m_downloadUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_downloadUrl));
    } else if (!m_releaseUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_releaseUrl));
    }
}

void UpdateController::onUpdateCheckFinished(bool hasUpdate, const iqtools::core::UpdateInfo& info) {
    m_hasUpdate = hasUpdate;
    m_latestVersion = info.version;
    m_releaseNotes = info.releaseNotes;
    m_releaseUrl = info.releaseUrl;
    m_downloadUrl = info.downloadUrl;

    if (hasUpdate) {
        m_statusMessage = QStringLiteral("发现新版本: %1").arg(info.version);
    } else {
        m_statusMessage = QStringLiteral("已是最新版本");
    }

    emit updateStatusChanged();
    emit statusMessageChanged();

    // Clear status message after 5 seconds
    QTimer::singleShot(5000, this, [this]() {
        if (m_statusMessage.startsWith(QStringLiteral("已是最新"))) {
            m_statusMessage.clear();
            emit statusMessageChanged();
        }
    });
}

void UpdateController::onUpdateCheckFailed(const QString& errorMessage) {
    m_hasUpdate = false;
    m_statusMessage = QStringLiteral("检查更新失败: %1").arg(errorMessage);
    emit updateStatusChanged();
    emit statusMessageChanged();
}

}  // namespace iqtools::app::bridge
