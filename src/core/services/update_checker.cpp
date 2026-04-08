#include "update_checker.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QVersionNumber>
#include <QtNetwork/QNetworkRequest>

#include "infra/logging/logger.h"

namespace iqtools::core {

// These macros are defined in CMakeLists.txt
#ifndef IQTOOLS_VERSION
#define IQTOOLS_VERSION "0.1.0"
#endif

#ifndef IQTOOLS_BUILD_DATE
#define IQTOOLS_BUILD_DATE __DATE__
#endif

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onNetworkReply);
}

UpdateChecker::~UpdateChecker() = default;

void UpdateChecker::checkForUpdates() {
    if (m_isChecking) {
        return;
    }

    m_isChecking = true;
    emit checkingChanged(true);

    QNetworkRequest request(QUrl(QString::fromLatin1(kGitHubApiUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("IQtools/%1").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_networkManager->get(request);

    iqtools::infra::logging::Logger::info(
        QStringLiteral("update"),
        QStringLiteral("Checking for updates..."));
}

QString UpdateChecker::currentVersion() {
    return QStringLiteral(IQTOOLS_VERSION);
}

QString UpdateChecker::buildDate() {
    return QStringLiteral(IQTOOLS_BUILD_DATE);
}

bool UpdateChecker::isChecking() const {
    return m_isChecking;
}

UpdateInfo UpdateChecker::lastUpdateInfo() const {
    return m_lastUpdateInfo;
}

void UpdateChecker::onNetworkReply(QNetworkReply* reply) {
    m_isChecking = false;
    emit checkingChanged(false);

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString errorMsg = reply->errorString();
        iqtools::infra::logging::Logger::warning(
            QStringLiteral("update"),
            QStringLiteral("Update check failed: %1").arg(errorMsg));
        emit updateCheckFailed(errorMsg);
        return;
    }

    const QByteArray data = reply->readAll();
    parseGitHubRelease(data);
}

bool UpdateChecker::isNewerVersion(const QString& remoteVersion) const {
    // Remove 'v' prefix if present
    QString remote = remoteVersion;
    if (remote.startsWith(QLatin1Char('v')) || remote.startsWith(QLatin1Char('V'))) {
        remote = remote.mid(1);
    }

    const QVersionNumber current = QVersionNumber::fromString(currentVersion());
    const QVersionNumber latest = QVersionNumber::fromString(remote);

    return latest > current;
}

void UpdateChecker::parseGitHubRelease(const QByteArray& data) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        const QString errorMsg = QStringLiteral("Failed to parse release info: %1")
                                     .arg(parseError.errorString());
        iqtools::infra::logging::Logger::warning(QStringLiteral("update"), errorMsg);
        emit updateCheckFailed(errorMsg);
        return;
    }

    const QJsonObject release = doc.object();
    
    m_lastUpdateInfo = UpdateInfo{};
    m_lastUpdateInfo.version = release.value(QStringLiteral("tag_name")).toString();
    m_lastUpdateInfo.releaseUrl = release.value(QStringLiteral("html_url")).toString();
    m_lastUpdateInfo.releaseNotes = release.value(QStringLiteral("body")).toString();
    m_lastUpdateInfo.publishedAt = release.value(QStringLiteral("published_at")).toString();

    // Find Windows download asset
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& assetVal : assets) {
        const QJsonObject asset = assetVal.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString().toLower();
        if (name.contains(QStringLiteral("windows")) || name.endsWith(QStringLiteral(".exe")) ||
            name.endsWith(QStringLiteral(".zip")) || name.endsWith(QStringLiteral(".msi"))) {
            m_lastUpdateInfo.downloadUrl = asset.value(QStringLiteral("browser_download_url")).toString();
            break;
        }
    }

    m_lastUpdateInfo.isNewerVersion = isNewerVersion(m_lastUpdateInfo.version);

    if (m_lastUpdateInfo.isNewerVersion) {
        iqtools::infra::logging::Logger::info(
            QStringLiteral("update"),
            QStringLiteral("New version available: %1 (current: %2)")
                .arg(m_lastUpdateInfo.version, currentVersion()));
    } else {
        iqtools::infra::logging::Logger::info(
            QStringLiteral("update"),
            QStringLiteral("Already up to date (current: %1, latest: %2)")
                .arg(currentVersion(), m_lastUpdateInfo.version));
    }

    emit updateCheckFinished(m_lastUpdateInfo.isNewerVersion, m_lastUpdateInfo);
}

}  // namespace iqtools::core
