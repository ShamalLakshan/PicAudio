/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioinput.h>

#include <cstdint>
#include <optional>

namespace PicAudioEngine {
[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::optional<int16_t> readOpusHeaderGainQ78(QIODevice* device);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT bool writeOpusHeaderGainQ78(QIODevice* device, int16_t gain);

enum class AudioFileFormat : uint8_t
{
    Unknown = 0,
    Mpeg,
    Aiff,
    Wav,
    Musepack,
    Ape,
    WavPack,
    Mp4,
    Flac,
    Vorbis,
    Opus,
    Asf,
    Dsf,
    Dsdiff,
};

class PICAUDIO_ENGINE_EXPORT TagLibReader : public AudioReader
{
public:
    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] bool canReadCover() const override;
    [[nodiscard]] bool canWriteMetaData() const override;

    [[nodiscard]] bool init(const AudioSource& source) override;
    [[nodiscard]] bool readTrack(const AudioSource& source, Track& track) override;
    [[nodiscard]] QByteArray readCover(const AudioSource& source, const Track& track, Track::Cover cover) override;
    [[nodiscard]] bool writeTrack(const AudioSource& source, const Track& track, WriteOptions options) override;
    [[nodiscard]] bool writeCover(const AudioSource& source, const Track& track, const TrackCovers& covers,
                                  WriteOptions options) override;

private:
    AudioFileFormat m_format{AudioFileFormat::Unknown};
    QString m_mimeType;
};
} // namespace PicAudioEngine
