/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioinput.h>
#include <core/engine/audioloader.h>

#include <QIODevice>

namespace PicAudioEngine {
class GeneralArchiveReader : public AudioReader
{
public:
    explicit GeneralArchiveReader(std::shared_ptr<AudioLoader> audioLoader);
    ~GeneralArchiveReader() override;

    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] bool canReadCover() const override;
    [[nodiscard]] bool canWriteMetaData() const override;
    [[nodiscard]] int subsongCount() const override;

    bool init(const AudioSource& source) override;

    [[nodiscard]] bool readTrack(const AudioSource& source, Track& track) override;
    [[nodiscard]] QByteArray readCover(const AudioSource& source, const Track& track, Track::Cover cover) override;
    [[nodiscard]] bool writeTrack(const AudioSource& source, const Track& track, WriteOptions options) override;

private:
    std::shared_ptr<AudioLoader> m_audioLoader;
    LoadedReader m_loadedReader;
};
} // namespace PicAudioEngine
