/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

namespace PicAudioEngine::Constants {
constexpr auto RecordSeparator = "\036";
constexpr auto UnitSeparator   = "\037";

constexpr auto PlayingIconMarker = u'\uE000';

constexpr auto InvalidGain        = -1000;
constexpr auto InvalidPeak        = -1;
constexpr auto OpusHeaderGainQ78  = "_OPUS_HEADER_GAIN_Q78";
constexpr auto RawRatingTagPrefix = "_FY_RATING_RAW_";
constexpr auto CueIndex01Sector   = "_CUE_INDEX01_SECTOR";
constexpr auto CueEndSector       = "_CUE_END_SECTOR";

namespace MetaData {
constexpr auto Title             = "TITLE";
constexpr auto Artist            = "ARTIST";
constexpr auto TrackArtist       = "TRACKARTIST";
constexpr auto UniqueArtist      = "UNIQUEARTIST";
constexpr auto Album             = "ALBUM";
constexpr auto AlbumArtist       = "ALBUMARTIST";
constexpr auto TrackNumber       = "TRACK";
constexpr auto TrackTotal        = "TRACKTOTAL";
constexpr auto Disc              = "DISC";
constexpr auto DiscTotal         = "DISCTOTAL";
constexpr auto Genre             = "GENRE";
constexpr auto Composer          = "COMPOSER";
constexpr auto Performer         = "PERFORMER";
constexpr auto Duration          = "DURATION";
constexpr auto DurationSecs      = "DURATION_S";
constexpr auto DurationMSecs     = "DURATION_MS";
constexpr auto Lyrics            = "LYRICS";
constexpr auto Comment           = "COMMENT";
constexpr auto Date              = "DATE";
constexpr auto Year              = "YEAR";
constexpr auto FileSize          = "FILESIZE";
constexpr auto FileSizeNatural   = "FILESIZE_NATURAL";
constexpr auto Bitrate           = "BITRATE";
constexpr auto SampleRate        = "SAMPLERATE";
constexpr auto FirstPlayed       = "FIRSTPLAYED";
constexpr auto LastPlayed        = "LASTPLAYED";
constexpr auto PlayCount         = "PLAYCOUNT";
constexpr auto Rating            = "RATING";
constexpr auto RatingNormalized  = "RATING_NORMALIZED";
constexpr auto Stars             = "STARS";
constexpr auto RatingStars       = "RATING_STARS";
constexpr auto RatingStarsPadded = "RATING_STARS_PADDED";
constexpr auto RatingEditor      = "RATING_EDITOR";
constexpr auto Loved             = "LOVED";
constexpr auto LoveEditor        = "LOVE_EDITOR";
constexpr auto Codec             = "CODEC";
constexpr auto CodecProfile      = "CODEC_PROFILE";
constexpr auto Tool              = "TOOL";
constexpr auto TagType           = "TAGTYPE";
constexpr auto Encoding          = "ENCODING";
constexpr auto Channels          = "CHANNELS";
constexpr auto BitDepth          = "BITDEPTH";
constexpr auto CreatedTime       = "CREATEDTIME";
constexpr auto AddedTime         = "ADDEDTIME";
constexpr auto LastModified      = "LASTMODIFIED";
constexpr auto FilePath          = "FILEPATH";
constexpr auto RelativePath      = "RELATIVEPATH";
constexpr auto FileName          = "FILENAME";
constexpr auto Extension         = "EXTENSION";
constexpr auto FileNameWithExt   = "FILENAME_EXT";
constexpr auto Directory         = "DIRECTORY";
constexpr auto Path              = "PATH";
constexpr auto Subsong           = "SUBSONG";
constexpr auto RGTrackGain       = "REPLAYGAIN_TRACK_GAIN";
constexpr auto RGTrackPeak       = "REPLAYGAIN_TRACK_PEAK";
constexpr auto RGTrackPeakDB     = "REPLAYGAIN_TRACK_PEAK_DB";
constexpr auto RGAlbumGain       = "REPLAYGAIN_ALBUM_GAIN";
constexpr auto RGAlbumPeak       = "REPLAYGAIN_ALBUM_PEAK";
constexpr auto RGAlbumPeakDB     = "REPLAYGAIN_ALBUM_PEAK_DB";
constexpr auto OpusHeaderGain    = "OPUS_HEADER_GAIN";
} // namespace MetaData

constexpr auto FrontCover    = "%frontcover%";
constexpr auto BackCover     = "%backcover%";
constexpr auto ArtistPicture = "%artistpicture%";
constexpr auto RatingEditor  = "%rating_editor%";
constexpr auto LoveEditor    = "%love_editor%";
} // namespace PicAudioEngine::Constants
