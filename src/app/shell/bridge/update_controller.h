#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace iqtools::core {
class UpdateChecker;
struct UpdateInfo;
}

namespace iqtools::app::bridge {

/// QML bridge for update checking functionality.
class UpdateController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
    Q_PROPERTY(bool hasUpdate READ hasUpdate NOTIFY updateStatusChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateStatusChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateStatusChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY updateStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit UpdateController(iqtools::core::UpdateChecker* checker, QObject* parent = nullptr);

    QString currentVersion() const;
    QString buildDate() const;
    bool checking() const;
    bool hasUpdate() const;
    QString latestVersion() const;
    QString releaseNotes() const;
    QString releaseUrl() const;
    QString statusMessage() const;

    Q_INVOKABLE void checkNow();
    Q_INVOKABLE void openReleasePage();
    Q_INVOKABLE void openDownloadPage();

signals:
    void checkingChanged();
    void updateStatusChanged();
    void statusMessageChanged();

private slots:
    void onUpdateCheckFinished(bool hasUpdate, const iqtools::core::UpdateInfo& info);
    void onUpdateCheckFailed(const QString& errorMessage);

private:
    iqtools::core::UpdateChecker* m_checker{nullptr};
    bool m_hasUpdate{false};
    QString m_latestVersion;
    QString m_releaseNotes;
    QString m_releaseUrl;
    QString m_downloadUrl;
    QString m_statusMessage;
};

}  // namespace iqtools::app::bridge
