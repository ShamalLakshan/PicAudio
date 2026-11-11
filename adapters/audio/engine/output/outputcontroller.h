/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audiooutput.h>

#include <functional>

class QObject;
class QString;

namespace PicAudioEngine {
class AudioPipeline;

/*!
 * Output backend helper for AudioEngine.
 *
 * Encapsulates output creation/device selection and pipeline output lifecycle
 * (init/uninit/pause-reset).
 */
class OutputController
{
public:
    OutputController(QObject* signalTarget, AudioPipeline* pipeline);

    //! Set factory used to create fresh output backend instances.
    void setOutputCreator(const OutputCreator& outputCreator);
    //! Set desired backend device id/name for next init.
    void setOutputDevice(const QString& device);
    //! Set runtime backend state callback for active output instances.
    void setOutputStateHandler(std::function<void(AudioOutput::State)> handler);
    //! Set runtime backend volume callback for active output instances.
    void setOutputVolumeHandler(std::function<void(double)> handler);

    [[nodiscard]] const QString& outputDevice() const;

    //! Create backend, attach to pipeline, and initialise for @p format.
    //! Returns false and emits engine error on failure.
    bool initOutput(const AudioFormat& format, double volume);
    //! Uninitialise current pipeline output backend.
    void uninitOutput();
    //! Immediate pause helper used by stop/pause/reconfigure flows.
    void pauseOutputImmediate(bool resetOutput);

private:
    QObject* m_signalTarget;
    AudioPipeline* m_pipeline;
    OutputCreator m_outputCreator;
    std::function<void(AudioOutput::State)> m_outputStateHandler;
    std::function<void(double)> m_outputVolumeHandler;
    QString m_outputDevice;
};
} // namespace PicAudioEngine
