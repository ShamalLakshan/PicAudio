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

#pragma once

#include "fycore_export.h"

#include "playbackordernavigator.h"

#include <core/player/playerdefs.h>

namespace Fooyin {
class FYCORE_EXPORT PlaybackCursor
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
} // namespace Fooyin
