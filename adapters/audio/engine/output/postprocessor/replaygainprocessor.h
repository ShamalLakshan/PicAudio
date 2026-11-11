/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "trackprocessor.h"

#include <core/engine/enginedefs.h>
#include <utils/snapshot.h>

namespace PicAudioEngine {
class SettingsManager;

/*!
 * Per-stream ReplayGain processor applied in `AudioMixer` before per-track DSP.
 *
 * Settings are read through a lock-free shared snapshot so UI/settings updates
 * can be consumed on the audio thread without blocking.
 */
class PICAUDIO_ENGINE_EXPORT ReplayGainProcessor : public TrackProcessor
{
public:
    enum class SelectionMode : uint8_t
    {
        Off = 0,
        Track,
        Album
    };

    struct RuntimeSettings
    {
        SelectionMode mode{SelectionMode::Off};
        Engine::RGProcessing processing{Engine::NoProcessing};
        double rgPreampDb{0.0};
        double nonRgPreampDb{0.0};
    };

    using SharedSettings    = Snapshot<RuntimeSettings>;
    using SharedSettingsPtr = std::shared_ptr<SharedSettings>;

    explicit ReplayGainProcessor(std::shared_ptr<const SharedSettings> settings);

    //! Create shared runtime snapshot storage for engine-wide reuse.
    static SharedSettingsPtr makeSharedSettings();
    //! Refresh shared snapshot from current user settings.
    static void refreshSharedSettings(const SettingsManager& settings, SharedSettings& sharedSettings);

    static Engine::ReplayGainOutputInfo outputInfo(const Track& track, const RuntimeSettings& settings);

    void init(const Track& track, const AudioFormat& format) override;
    [[nodiscard]] bool isActive() const override;
    void process(double* samples, size_t sampleCount) override;
    void endOfTrack() override;
    void reset() override;

private:
    void refreshSettings();
    void updateGain();
    [[nodiscard]] double calculateGain() const;

    std::shared_ptr<const SharedSettings> m_settings;
    uint64_t m_settingsEpoch;

    Track m_track;
    SelectionMode m_mode;
    Engine::RGProcessing m_processing;

    double m_rgPreampDb;
    double m_nonRgPreampDb;

    double m_linearGain;
    bool m_active;
};
} // namespace PicAudioEngine
