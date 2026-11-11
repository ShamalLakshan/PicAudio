/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/coresettings.h>

#include <QDialog>

class QSpinBox;

namespace PicAudioEngine::Pipewire {
constexpr auto LatencySetting = "PipeWire/LatencyMs";
constexpr auto DefaultLatency = 20;
constexpr auto MaximumLatency = 200;

class PipewireSettings : public QDialog
{
    Q_OBJECT

public:
    explicit PipewireSettings(QWidget* parent = nullptr);

    void accept() override;

private:
    PicAudioSettings m_settings;
    QSpinBox* m_latency;
};
} // namespace PicAudioEngine::Pipewire
