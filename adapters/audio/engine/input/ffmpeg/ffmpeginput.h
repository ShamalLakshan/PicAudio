/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioinput.h>

#include "ffmpegframe.h"

struct AVFormatContext;

namespace PicAudioEngine {
class AudioFormat;
class AudioBuffer;
class FFmpegInputPrivate;
class FFmpegReaderPrivate;

class PICAUDIO_ENGINE_EXPORT FFmpegDecoder : public AudioDecoder
{
public:
    FFmpegDecoder();
    ~FFmpegDecoder() override;

    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] bool supportsRemoteSources() const override;
    [[nodiscard]] int bitrate() const override;
    [[nodiscard]] bool trackHasChanged() const override;
    [[nodiscard]] Track changedTrack() const override;
    [[nodiscard]] std::optional<TimedTrackChange> takeTimedTrackChange() override;

    //! Apply properties of the selected decoded stream to an existing track.
    void applyStreamProperties(Track& track) const;

    std::optional<AudioFormat> init(const AudioSource& source, const Track& track, DecoderOptions options) override;
    void start() override;
    void stop() override;

    [[nodiscard]] bool isSeekable() const override;
    void seek(uint64_t pos) override;

    Frame readFrame();
    ReadResult readAudio(size_t bytes) override;
    AudioBuffer readBuffer(size_t bytes) override;

    [[nodiscard]] std::optional<bool> isPlanar() const;

private:
    std::unique_ptr<FFmpegInputPrivate> p;
};

class PICAUDIO_ENGINE_EXPORT FFmpegReader : public AudioReader,
                                   public CancellableReader
{
public:
    FFmpegReader();
    ~FFmpegReader() override;

    void setReadCancellationToken(std::stop_token token) override;

    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] bool supportsRemoteSources() const override;
    [[nodiscard]] bool canReadCover() const override;
    [[nodiscard]] bool canWriteMetaData() const override;

    [[nodiscard]] int subsongCount() const override;
    [[nodiscard]] bool init(const AudioSource& source) override;
    [[nodiscard]] bool readTrack(const AudioSource& source, Track& track) override;
    [[nodiscard]] QByteArray readCover(const AudioSource& source, const Track& track, Track::Cover cover) override;
    [[nodiscard]] bool writeTrack(const AudioSource& source, const Track& track, WriteOptions options) override;

private:
    std::unique_ptr<FFmpegReaderPrivate> p;
};
} // namespace PicAudioEngine
