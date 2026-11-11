/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audiooutput.h>
#include <core/engine/enginedefs.h>
#include <core/engine/levelframe.h>
#include <core/engine/pcmframe.h>

#include <QByteArray>
#include <QObject>

namespace PicAudioEngine {
struct AudioOutputBuilder;
class AudioBuffer;
class VisualisationService;

using OutputNames = std::vector<QString>;

/*!
 * High-level playback engine facade exposed to app/UI layers.
 *
 * Implementations bridge UI/player requests to an underlying audio engine and
 * emit normalised engine events/signals for the rest of the application.
 */
class PICAUDIO_ENGINE_EXPORT EngineController : public QObject
{
    Q_OBJECT

public:
    explicit EngineController(QObject* parent = nullptr)
        : QObject{parent}
    { }

    //! Current playback state as reported by the engine.
    [[nodiscard]] virtual Engine::PlaybackState engineState() const = 0;
    //! All registered output backend names.
    [[nodiscard]] virtual OutputNames getAllOutputs() const = 0;
    //! Available devices for a specific output backend.
    [[nodiscard]] virtual OutputDevices getOutputDevices(const QString& output) const = 0;
    //! Apply a combined output/device/DSP/bitdepth profile.
    virtual void applyOutputProfile(const Engine::OutputProfileRequest& request) = 0;
    /*!
     * Pull-based visualisation API backed by shared post-master-DSP,
     * pre-output-fader PCM history.
     *
     * This is intended for in-app visual widgets that query PCM or spectrum
     * windows on their own repaint schedule.
     *
     * For push-style consumers such as meters, use the analysis signals below instead.
     */
    [[nodiscard]] virtual VisualisationService* visualisationService() const = 0;

    /*!
     * Registers an audio output backend factory under @p name.
     * @note @p name must be unique.
     */
    virtual void addOutput(const QString& name, OutputCreator output) = 0;

Q_SIGNALS:
    //! Emitted when output backend selection changes.
    void outputChanged(const QString& output, const QString& device);
    //! Emitted when device for current backend changes.
    void deviceChanged(const QString& device);

    //! Fatal/non-recoverable engine error text.
    void engineError(const QString& error);
    //! Playback state transitions.
    void engineStateChanged(PicAudioEngine::Engine::PlaybackState state);
    //! Emitted while a faded pause drains already-queued audio to the output.
    void audiblePauseDrainStarted();
    //! Emitted once the faded-pause output drain is no longer audible.
    void audiblePauseDrainCompleted();
    //! Track-status transitions with stable generation identifier.
    void trackStatusContextChanged(const PicAudioEngine::Engine::TrackStatusContext& context);

    //! Push-style fixed-hop level analysis snapshots for general consumers.
    void levelReady(const PicAudioEngine::LevelFrame& frame);
    //! Push-style fixed-hop PCM analysis snapshots for general consumers.
    void pcmReady(const PicAudioEngine::PcmFrame& frame);

    //! Metadata/context update for currently active track.
    void trackChanged(const PicAudioEngine::Track& track);
    //! Logical track switch committed by the engine.
    void trackCommitted(const PicAudioEngine::Engine::TrackCommitContext& context);
    //! Early callback before track reaches terminal end.
    void trackAboutToFinish(const PicAudioEngine::Engine::AboutToFinishContext& context);
    //! Callback fired when transition timing reaches the switch anchor.
    void trackReadyToSwitch(const PicAudioEngine::Engine::AboutToFinishContext& context);
    //! Callback fired when active logical track reaches its exact boundary.
    void trackBoundaryReached(const PicAudioEngine::Engine::AboutToFinishContext& context);
    //! Prepared-next-track readiness notification.
    void nextTrackReadiness(const PicAudioEngine::Engine::PlaybackItem& item, bool ready, uint64_t requestId);
    //! Result of async prepared-crossfade arm attempt for a specific boundary generation.
    void preparedCrossfadeArmResult(const PicAudioEngine::Engine::PlaybackItem& item, uint64_t generation, bool armed);
    //! Result of async prepared-gapless arm attempt for a specific boundary generation.
    void preparedGaplessArmResult(const PicAudioEngine::Engine::PlaybackItem& item, uint64_t generation, bool armed);

    //! Emitted after stop teardown completes.
    void finished();
};
} // namespace PicAudioEngine
