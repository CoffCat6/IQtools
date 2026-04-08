#pragma once

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace iqtools::infra::storage {

/// Low-level YAML settings file reader/writer.
/// Handles a flat key-value YAML format (no nested structures).
/// Thread-safe for single-writer usage patterns.
class SettingsStorage {
public:
    explicit SettingsStorage(const QString& filePath);

    /// Load settings from YAML file. Returns true on success.
    bool load();

    /// Save current values to YAML file. Returns true on success.
    bool save() const;

    /// Check whether the settings file exists.
    bool exists() const;

    /// Get a value by key, with optional default.
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /// Set a value by key.
    void setValue(const QString& key, const QVariant& value);

    /// Get all key-value pairs.
    QMap<QString, QVariant> allValues() const;

    /// Replace all values.
    void setAllValues(const QMap<QString, QVariant>& values);

    /// Get the file path.
    QString filePath() const;

private:
    /// Parse a simple YAML line into key-value pair.
    static bool parseLine(const QString& line, QString& key, QString& rawValue);

    /// Convert a raw YAML value string into a QVariant (bool, int, double, string).
    static QVariant parseValue(const QString& rawValue);

    /// Serialize a QVariant into a YAML value string.
    static QString serializeValue(const QVariant& value);

private:
    QString m_filePath;
    QMap<QString, QVariant> m_values;
};

}  // namespace iqtools::infra::storage
