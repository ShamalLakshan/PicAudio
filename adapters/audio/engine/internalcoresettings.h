/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <utils/settings/settingsentry.h>

#include <QStringList>
#include <memory>

namespace PicAudioEngine {
class SettingsManager;
class DbConnectionPool;
using DbConnectionPoolPtr = std::shared_ptr<DbConnectionPool>;

enum class Id3v2WriteVersion : uint8_t
{
    V3 = 3,
    V4 = 4,
};

enum class Mp3TagWritingScheme : uint8_t
{
    Id3v2AndId3v1 = 0,
    Id3v2,
    ApeAndId3v1,
    Ape,
};

enum class ReplayGainType : uint8_t
{
    Track = 0,
    Album,
    // Track gain if shuffle, else album gain
    PlaybackOrder
};

namespace Settings::Core::Internal {
Q_NAMESPACE_EXPORT(PICAUDIO_ENGINE_EXPORT)

constexpr auto PlaylistSaveMetadata              = "Playlist/SaveMetadata";
constexpr auto PlaylistSavePathType              = "Playlist/SavePathType";
constexpr auto AutoExportPlaylists               = "Playlist/AutoExport";
constexpr auto AutoExportPlaylistsType           = "Playlist/AutoExportType";
constexpr auto AutoExportPlaylistsPath           = "Playlist/AutoExportPath";
constexpr auto AutoExportPlaylistsRemove         = "Playlist/AutoExportRemove";
constexpr auto AutoExportPlaylistsSaveRemoved    = "Playlist/AutoExportSaveRemoved";
constexpr auto MarkUnavailable                   = "Library/MarkUnavailable";
constexpr auto MarkUnavailableStartup            = "Library/MarkUnavailableOnStartup";
constexpr auto SaveActivePlaylistState           = "Playlist/SaveActivePlaylistState";
constexpr auto SavePlaybackState                 = "Player/SavePlaybackState";
constexpr auto LibraryRestrictTypes              = "Library/RestrictTypes";
constexpr auto LibraryExcludeTypes               = "Library/ExcludeTypes";
constexpr auto ExternalRestrictTypes             = "Library/ExternalRestrictTypes";
constexpr auto ExternalExcludeTypes              = "Library/ExternalExcludeTypes";
constexpr auto FFmpegAllExtensions               = "Engine/FFmpegAllExtensions";
constexpr auto FFmpegPriorityExtensions          = "Engine/FFmpegPriorityExtensions";
constexpr auto ReaderProbeAllExtensions          = "Engine/ReaderProbeAllExtensions";
constexpr auto SplitId3v23SemicolonSeparatedTags = "Tagging/SplitId3v23SemicolonSeparatedTags";
constexpr auto Id3v2WriteVersion                 = "Tagging/Id3v2WriteVersion";
constexpr auto Mp3TagWritingScheme               = "Tagging/Mp3TagWritingScheme";

constexpr auto ExtraTagCompressionMinSize       = "Library/ExtraTagCompressionMinSizeBytes";
constexpr auto ExtraTagCompressionLevel         = "Library/ExtraTagCompressionLevel";
constexpr auto ExtraTagCompressionAlways        = "Library/ExtraTagCompressionAlways";
constexpr auto ExtraTagCompressionNever         = "Library/ExtraTagCompressionNever";
constexpr int DefaultExtraTagCompressionMinSize = 256;
constexpr int DefaultExtraTagCompressionLevel   = 1;

constexpr auto DefaultRemoteReadAheadKb   = 2048;
constexpr auto DefaultRemotePrebufferMs   = 0;
constexpr auto DefaultRemoteOpenTimeoutMs = 8000;

constexpr auto DefaultAutoCrossfadeAlbumScript = "$if(%album%,%albumartist%|%date%|%album%,)";

PICAUDIO_ENGINE_EXPORT QStringList defaultFFmpegPriorityExtensions();
PICAUDIO_ENGINE_EXPORT QStringList defaultReaderProbeAllExtensions();
PICAUDIO_ENGINE_EXPORT bool shouldProbeAllReadersForFile(const QString& file);
PICAUDIO_ENGINE_EXPORT QStringList defaultOutputResamplerPreference();

enum CoreInternalSettings : uint32_t
{
    MonitorLibraryDirectories = 0 | Type::Bool,
    MuteVolume                = 1 | Type::Double,
    DisabledPlugins           = 2 | Type::StringList,
    EngineFading              = 3 | Type::Bool,
    FadingValues              = 4 | Type::Variant,
    EngineCrossfading         = 5 | Type::Bool,
    CrossfadingValues         = 6 | Type::Variant,
    VBRUpdateInterval         = 7 | Type::Int,
    ProxyMode                 = 8 | Type::Int,
    ProxyType                 = 9 | Type::Int,
    ProxyHost                 = 10 | Type::String,
    ProxyPort                 = 11 | Type::Int,
    ProxyAuth                 = 12 | Type::Bool,
    ProxyUsername             = 13 | Type::String,
    ProxyPassword             = 14 | Type::String,
    DecodeLowWatermarkRatio   = 15 | Type::Double,
    DecodeHighWatermarkRatio  = 16 | Type::Double,
    CrossfadeSwitchPolicy     = 17 | Type::Int,
    OutputDeviceProfiles      = 18 | Type::Variant,
    OpusHeaderWriteMode       = 19 | Type::Int,
    RemoteReadAheadKb         = 20 | Type::Int,
    RemoteBufferLengthMs      = 21 | Type::Int,
    RemotePrebufferMs         = 22 | Type::Int,
    RemoteOpenTimeoutMs       = 23 | Type::Int,
    MonitorTrackFiles         = 24 | Type::Bool,
    OutputAutoResample        = 25 | Type::Bool,
    OutputResamplerPreference = 26 | Type::StringList,
    PlaylistSkipUnavailable   = 27 | Type::Bool,
    ReplayGainLastActiveMode  = 28 | Type::Int,
    SkipSameAlbumCrossfade    = 29 | Type::Bool,
    AutoCrossfadeAlbumScript  = 30 | Type::String,
};
Q_ENUM_NS(CoreInternalSettings)
} // namespace Settings::Core::Internal

class PICAUDIO_ENGINE_EXPORT CoreSettings
{
public:
    explicit CoreSettings(SettingsManager* settingsManager, DbConnectionPoolPtr dbPool = {});
    ~CoreSettings();

private:
    SettingsManager* m_settings;
};
} // namespace PicAudioEngine
