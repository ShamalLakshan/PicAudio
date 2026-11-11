/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/playlist/playlist.h>

#include <optional>
#include <unordered_set>

namespace PicAudioEngine {
/*!
 * Represents a single track move within a playlist patch.
 * The move is applied against the current playlist state at the time the
 * operation is executed.
 */
struct PICAUDIO_ENGINE_EXPORT PlaylistTrackMove
{
    /** Stable playlist entry identity of the track to move. */
    UId entryId;
    /** Destination playlist index after the move is applied. */
    int targetIndex{-1};

    [[nodiscard]] bool isValid() const
    {
        return entryId.isValid() && targetIndex >= 0;
    }
};

/*!
 * Represents a contiguous group of tracks to insert into a playlist patch.
 * @note @c index is the destination playlist index before the insertion is applied.
 */
struct PICAUDIO_ENGINE_EXPORT PlaylistTrackInsertion
{
    /** Destination playlist index for the first inserted track. */
    int index{-1};
    /** Tracks to insert, in final order. */
    PlaylistTrackList tracks;

    [[nodiscard]] bool isValid() const
    {
        return index >= 0 && !tracks.empty();
    }
};

/*!
 * Describes an incremental playlist update.
 *
 * The operations are interpreted in this order:
 * 1. remove @c removedEntries
 * 2. apply @c insertions
 * 3. apply @c moves
 * 4. refresh @c updatedEntries
 *
 * If @c requiresReset is set, callers should ignore the incremental fields and
 * rebuild the playlist model from scratch instead.
 */
struct PICAUDIO_ENGINE_EXPORT PlaylistChangeset
{
    /** Playlist entries to remove from the pre-change playlist. */
    std::vector<UId> removedEntries;
    /** Contiguous insertion groups to apply after removals. */
    std::vector<PlaylistTrackInsertion> insertions;
    /** Move operations to apply after insertions. */
    std::vector<PlaylistTrackMove> moves;
    /** Playlist entries to refresh after structural changes are applied. */
    std::vector<UId> updatedEntries;
    /** Whether the resulting playlist retains no entry identities from the previous contents. */
    bool replacesAllEntries{false};
    /** Whether the change is too large or ambiguous for incremental application. */
    bool requiresReset{false};

    /** Returns @c true when there is no structural or metadata change to apply. */
    [[nodiscard]] bool isEmpty() const
    {
        return !requiresReset && removedEntries.empty() && insertions.empty() && moves.empty()
            && updatedEntries.empty();
    }
};

using TrackEntryIdSet = std::unordered_set<UId, UId::UIdHash>;

/*!
 * Builds a playlist patch that transforms @p oldTracks into @p newTracks.
 *
 * Tracks are matched by stable playlist @c entryId.
 *
 * @param updatedTrackEntries Optional set of matched entry ids that should be
 * treated as metadata updates even if the old and new track objects compare equal.
 */
[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::optional<PlaylistChangeset>
buildPlaylistChangeset(const PlaylistTrackList& oldTracks, const PlaylistTrackList& newTracks,
                       const TrackEntryIdSet& updatedTrackEntries = {});
} // namespace PicAudioEngine

Q_DECLARE_METATYPE(PicAudioEngine::PlaylistTrackMove)
Q_DECLARE_METATYPE(PicAudioEngine::PlaylistTrackInsertion)
Q_DECLARE_METATYPE(PicAudioEngine::PlaylistChangeset)
