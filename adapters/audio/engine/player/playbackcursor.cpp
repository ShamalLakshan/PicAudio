/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playbackcursor.h"

namespace PicAudioEngine {
PlaybackCursor::PlaybackCursor()
    : m_boundaryStop{BoundaryStop::None}
{ }

void PlaybackCursor::reset()
{
    m_boundaryStop = BoundaryStop::None;
}

void PlaybackCursor::onTrackCommitted()
{
    reset();
}

void PlaybackCursor::onSeek()
{
    reset();
}

void PlaybackCursor::stopAt(BoundaryStop boundary)
{
    m_boundaryStop = boundary;
}

PlaybackOrderNavigator::RestartTarget PlaybackCursor::restartTargetFor(Command command, Player::PlayState state) const
{
    if(state != Player::PlayState::Stopped) {
        return PlaybackOrderNavigator::RestartTarget::None;
    }

    switch(command) {
        case Command::Play:
        case Command::Next:
            if(m_boundaryStop == BoundaryStop::End) {
                return PlaybackOrderNavigator::RestartTarget::Beginning;
            }
            break;
        case Command::Previous:
            if(m_boundaryStop == BoundaryStop::Start) {
                return PlaybackOrderNavigator::RestartTarget::End;
            }
            break;
    }

    return PlaybackOrderNavigator::RestartTarget::None;
}
} // namespace PicAudioEngine
