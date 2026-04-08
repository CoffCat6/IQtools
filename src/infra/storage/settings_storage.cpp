#include "settings_storage.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QTextStream>

namespace iqtools::infra::storage {

SettingsStorage::SettingsStorage(const QString& filePath)
    : m_filePath(filePath)
{
}

bool SettingsStorage::load()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QMap<QString, QVariant> loaded;

    while (!in.atEnd()) {
        const QString line = in.readLine();

        // Skip empty lines and comments
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) {
            continue;
        }

        QString key;
        QString rawValue;
        if (parseLine(trimmed, key, rawValue)) {
            loaded.insert(key, parseValue(rawValue));
        }
    }

    m_values = loaded;
    return true;
}

bool SettingsStorage::save() const
{
    const QFileInfo info(m_filePath);
    const QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << QStringLiteral("# IQtools Settings\n");
    out << QStringLiteral("# Auto-generated — edit with care\n\n");

    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        out << it.key() << QStringLiteral(": ") << serializeValue(it.value()) << QStringLiteral("\n");
    }

    return file.commit();
}

bool SettingsStorage::exists() const
{
    return QFile::exists(m_filePath);
}

QVariant SettingsStorage::value(const QString& key, const QVariant& defaultValue) const
{
    return m_values.value(key, defaultValue);
}

void SettingsStorage::setValue(const QString& key, const QVariant& value)
{
    m_values.insert(key, value);
}

QMap<QString, QVariant> SettingsStorage::allValues() const
{
    return m_values;
}

void SettingsStorage::setAllValues(const QMap<QString, QVariant>& values)
{
    m_values = values;
}

QString SettingsStorage::filePath() const
{
    return m_filePath;
}

bool SettingsStorage::parseLine(const QString& line, QString& key, QString& rawValue)
{
    const int colonIndex = line.indexOf(QLatin1Char(':'));
    if (colonIndex <= 0) {
        return false;
    }

    key = line.left(colonIndex).trimmed();
    rawValue = line.mid(colonIndex + 1).trimmed();
    return !key.isEmpty();
}

QVariant SettingsStorage::parseValue(const QString& rawValue)
{
    // Boolean
    if (rawValue.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (rawValue.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
        return false;
    }

    // Integer
    bool intOk = false;
    const int intVal = rawValue.toInt(&intOk);
    if (intOk) {
        return intVal;
    }

    // Double
    bool doubleOk = false;
    const double dblVal = rawValue.toDouble(&doubleOk);
    if (doubleOk) {
        return dblVal;
    }

    // Strip surrounding quotes if present
    if (rawValue.length() >= 2) {
        if ((rawValue.startsWith(QLatin1Char('"')) && rawValue.endsWith(QLatin1Char('"')))
            || (rawValue.startsWith(QLatin1Char('\'')) && rawValue.endsWith(QLatin1Char('\'')))) {
            return rawValue.mid(1, rawValue.length() - 2);
        }
    }

    return rawValue;
}

QString SettingsStorage::serializeValue(const QVariant& value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::Int:
    case QMetaType::LongLong:
        return QString::number(value.toLongLong());
    case QMetaType::Double:
    case QMetaType::Float:
        return QString::number(value.toDouble());
    default:
        break;
    }

    const QString str = value.toString();
    // Quote strings that contain special YAML characters
    if (str.contains(QLatin1Char(':')) || str.contains(QLatin1Char('#'))
        || str.contains(QLatin1Char('\'')) || str.contains(QLatin1Char('"'))
        || str.startsWith(QLatin1Char(' ')) || str.endsWith(QLatin1Char(' '))) {
        return QLatin1Char('"') + str + QLatin1Char('"');
    }

    return str;
}

}  // namespace iqtools::infra::storage
