/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "playbackordernavigator.h"

#include <core/player/playerdefs.h>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PlaybackCursor
{
public:
    enum class Command : uint8_t
    {
        Play = 0,
        Previous,
        Next,
    };

    enum class BoundaryStop : uint8_t
    {
        None = 0,
        Start,
        End,
    };

    PlaybackCursor();

    void reset();

    void onTrackCommitted();
    void onSeek();
    void stopAt(BoundaryStop boundary);

    [[nodiscard]] PlaybackOrderNavigator::RestartTarget restartTargetFor(Command command,
                                                                         Player::PlayState state) const;

private:
    BoundaryStop m_boundaryStop;
};
} // namespace PicAudioEngine
