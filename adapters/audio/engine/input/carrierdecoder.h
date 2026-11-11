/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioinput.h>

#include <memory>

namespace PicAudioEngine {
class CarrierDecoderPrivate;

class PICAUDIO_ENGINE_EXPORT CarrierDecoder final : public AudioDecoder
{
public:
    explicit CarrierDecoder(std::unique_ptr<AudioDecoder> decoder);
    ~CarrierDecoder() override;

    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] QStringList preferredExtensions() const override;
    [[nodiscard]] QStringList supportedSchemes() const override;
    [[nodiscard]] bool supportsRemoteSources() const override;
    [[nodiscard]] bool needsMoreInput() const override;
    [[nodiscard]] bool isSeekable() const override;
    [[nodiscard]] bool allowsConcurrentDecoding() const override;
    [[nodiscard]] int playbackPrebufferMs() const override;
    [[nodiscard]] QStringList takeWarnings() override;
    [[nodiscard]] RepeatHandling repeatHandling() const override;
    [[nodiscard]] bool trackHasChanged() const override;
    [[nodiscard]] Track changedTrack() const override;
    [[nodiscard]] std::optional<TimedTrackChange> takeTimedTrackChange() override;
    [[nodiscard]] int bitrate() const override;

    std::optional<AudioFormat> init(const AudioSource& source, const Track& track, DecoderOptions options) override;
    void start() override;
    void stop() override;
    void seek(uint64_t pos) override;
    ReadResult readAudio(size_t bytes) override;
    AudioBuffer readBuffer(size_t bytes) override;

protected:
    void playbackHintsChanged(PlaybackHints hints) override;
    void interruptRead() override;

private:
    std::unique_ptr<CarrierDecoderPrivate> p;
};
} // namespace PicAudioEngine
