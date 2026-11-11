/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

struct AVStream;

namespace PicAudioEngine {
class Stream
{
public:
    Stream();
    explicit Stream(AVStream* stream);

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] AVStream* avStream() const;
    [[nodiscard]] int index() const;

private:
    AVStream* m_stream;
};
} // namespace PicAudioEngine
