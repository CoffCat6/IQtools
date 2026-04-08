#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace iqtools::core {

/// Update information structure.
struct UpdateInfo {
    QString version;           // e.g., "0.2.0"
    QString releaseUrl;        // GitHub release page URL
    QString downloadUrl;       // Direct download URL
    QString releaseNotes;      // Release notes / changelog
    QString publishedAt;       // Release date
    bool isNewerVersion{false};
};

/// Application update checker.
/// Checks for new versions via GitHub Releases API.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    /// Start checking for updates asynchronously.
    void checkForUpdates();

    /// Get the current application version.
    static QString currentVersion();

    /// Get the application build date.
    static QString buildDate();

    /// Check if currently checking for updates.
    bool isChecking() const;

    /// Get the last check result (empty if no check performed).
    UpdateInfo lastUpdateInfo() const;

signals:
    /// Emitted when update check completes.
    void updateCheckFinished(bool hasUpdate, const UpdateInfo& info);

    /// Emitted when update check fails.
    void updateCheckFailed(const QString& errorMessage);

    /// Emitted when checking state changes.
    void checkingChanged(bool checking);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    bool isNewerVersion(const QString& remoteVersion) const;
    void parseGitHubRelease(const QByteArray& data);

private:
    QNetworkAccessManager* m_networkManager{nullptr};
    bool m_isChecking{false};
    UpdateInfo m_lastUpdateInfo;

    static constexpr const char* kGitHubApiUrl =
        "https://api.github.com/repos/pumpkin-pieman/IQtools/releases/latest";
};

}  // namespace iqtools::core
