/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/coresettings.h>

#include <QDialog>

class QSpinBox;

namespace PicAudioEngine {
constexpr auto BufferLengthSetting = "ALSA/BufferLength";
constexpr auto DefaultBufferLength = 200;
constexpr auto PeriodLengthSetting = "ALSA/PeriodLength";
constexpr auto DefaultPeriodLength = 40;

class AlsaSettings : public QDialog
{
    Q_OBJECT

public:
    explicit AlsaSettings(QWidget* parent = nullptr);

    void accept() override;

private:
    PicAudioSettings m_settings;
    QSpinBox* m_bufferLength;
    QSpinBox* m_periodLength;
};
} // namespace PicAudioEngine
