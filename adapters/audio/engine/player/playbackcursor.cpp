/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "playbackcursor.h"

namespace Fooyin {
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
} // namespace Fooyin
