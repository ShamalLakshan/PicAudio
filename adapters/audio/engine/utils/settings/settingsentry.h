/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QVariant>

namespace PicAudioEngine {
namespace Settings {
enum Type : uint32_t
{
    Variant    = 0 << 28,
    Bool       = 1 << 28,
    Int        = 2 << 28,
    Float      = 3 << 28,
    Double     = 4 << 28,
    String     = 5 << 28,
    StringList = 6 << 28,
    ByteArray  = 7 << 28,
};
} // namespace Settings

class PICAUDIO_UTILS_EXPORT SettingsEntry : public QObject
{
    Q_OBJECT

public:
    SettingsEntry(QString name, const QVariant& value, QObject* parent = nullptr);
    SettingsEntry(QString name, const QVariant& value, Settings::Type type, QObject* parent = nullptr);

    [[nodiscard]] QString key() const;
    [[nodiscard]] Settings::Type type() const;
    [[nodiscard]] QVariant value() const;
    [[nodiscard]] QVariant defaultValue() const;
    [[nodiscard]] bool isTemporary() const;
    [[nodiscard]] bool wasChanged() const;

    bool setValue(const QVariant& value);
    bool setValueSilently(const QVariant& value);
    void setIsTemporary(bool isTemporary);
    void notifySubscribers(const QVariant& value);

    bool reset();

Q_SIGNALS:
    void settingChangedVariant(const QVariant& value);
    void settingChangedBool(bool value);
    void settingChangedInt(int value);
    void settingChangedFloat(float value);
    void settingChangedDouble(double value);
    void settingChangedString(const QString& value);
    void settingChangedStringList(const QStringList& value);
    void settingChangedByteArray(const QByteArray& value);

private:
    QString m_key;
    Settings::Type m_type;
    QVariant m_value;
    QVariant m_defaultValue;
    bool m_isTemporary;
    bool m_wasChanged;
};
} // namespace PicAudioEngine
