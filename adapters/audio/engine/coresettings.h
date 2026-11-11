/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <utils/settings/settingsentry.h>

#include <QNetworkProxy>
#include <QObject>
#include <QSettings>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PicAudioSettings : public QSettings
{
public:
    explicit PicAudioSettings(QObject* parent = nullptr);
};

class PICAUDIO_ENGINE_EXPORT PicAudioStateSettings : public QSettings
{
public:
    explicit PicAudioStateSettings(QObject* parent = nullptr);
};

namespace Settings::Core {
Q_NAMESPACE_EXPORT(PICAUDIO_ENGINE_EXPORT)

const auto PlayModeKey     = "Player/PlayMode";
const auto SearchModeKey   = "Searching/SearchMode";
const auto SearchScriptKey = "Searching/SearchScript";

enum CoreSettings : uint32_t
{
    Version                       = 1 | Type::String,
    FirstRun                      = 2 | Type::Bool,
    PlayMode                      = 3 | Type::Int,
    AutoRefresh                   = 4 | Type::Bool,
    LibrarySortScript             = 5 | Type::String,
    AudioOutput                   = 6 | Type::String,
    OutputVolume                  = 7 | Type::Double,
    RewindPreviousTrack           = 8 | Type::Bool,
    GaplessPlayback               = 9 | Type::Bool,
    Language                      = 10 | Type::String,
    BufferLength                  = 11 | Type::Int,
    OpenFilesPlaylist             = 12 | Type::String,
    OpenFilesSendTo               = 13 | Type::Bool,
    SaveRatingToMetadata          = 14 | Type::Bool,
    SavePlaycountToMetadata       = 15 | Type::Bool,
    PlayedThreshold               = 16 | Type::Double,
    ExternalSortScript            = 17 | Type::String,
    Shutdown                      = 18 | Type::Bool,
    StopAfterCurrent              = 19 | Type::Bool,
    RGMode                        = 20 | Type::Int,
    RGType                        = 21 | Type::Int,
    RGPreAmp                      = 22 | Type::Float,
    NonRGPreAmp                   = 23 | Type::Float,
    UseVariousForCompilations     = 24 | Type::Bool,
    ShuffleAlbumsGroupScript      = 25 | Type::String,
    ShuffleAlbumsSortScript       = 26 | Type::String,
    ActiveTrack                   = 27 | Type::Variant,
    ActiveTrackId                 = 28 | Type::Int,
    FollowPlaybackQueue           = 29 | Type::Bool,
    StopIfActivePlaylistDeleted   = 30 | Type::Bool,
    ResetStopAfterCurrent         = 31 | Type::Bool,
    PreserveTimestamps            = 32 | Type::Bool,
    PlaylistSkipMissing           = 33 | Type::Bool,
    PlaybackQueueStopWhenFinished = 34 | Type::Bool,
    OutputBitDepth                = 35 | Type::Int,
    OutputDither                  = 36 | Type::Bool,
    ClearPlaybackQueueOnStartup   = 37 | Type::Bool,
    LibraryViewPlaylistSortScript = 38 | Type::String,
    OverwriteRatingOnReload       = 39 | Type::Bool,
    OverwritePlaycountOnReload    = 40 | Type::Bool,
    OpenFileAddDirectory          = 41 | Type::Bool,
    AddFoldersIgnorePlaylists     = 42 | Type::Bool,
    PlaylistPreventDuplicates     = 43 | Type::Bool,
    PlayedThresholdTime           = 44 | Type::Int,
    PlaybackQueueMode             = 45 | Type::Int,
    PlaybackQueueHistoryLimit     = 46 | Type::Int,
    PlaybackQueuePlayNowAction    = 47 | Type::Int,
    VolumeControlEnabled          = 48 | Type::Bool,
};
Q_ENUM_NS(CoreSettings)
} // namespace Settings::Core
} // namespace PicAudioEngine
