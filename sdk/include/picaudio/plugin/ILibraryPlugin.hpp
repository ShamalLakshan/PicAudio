#pragma once

#include "Defines.hpp"

namespace picaudio::plugin {

/**
 * @brief Extension interface for library services (lyrics fetching, metadata
 * enrichment, scrobbling).
 */
class ILibraryPlugin {
public:
  virtual ~ILibraryPlugin() = default;

  /**
   * @brief Enrich track metadata tags.
   * @param inTrack Source track metadata.
   * @param outEnriched Populated enriched metadata buffer.
   * @return true if enrichment succeeded, false otherwise.
   */
  virtual bool enrichMetadata(const PicAudioTrackInfo *inTrack,
                              PicAudioTrackInfo *outEnriched) {
    (void)inTrack;
    (void)outEnriched;
    return false;
  }

  /**
   * @brief Retrieve synchronized or plain text lyrics for a track.
   * @param artist Artist name.
   * @param title Track title.
   * @param outBuffer Output buffer for lyrics text or LRC stream.
   * @param maxLen Capacity in bytes of outBuffer.
   * @return Number of bytes written (excluding null terminator), or 0 if not
   * found.
   */
  virtual size_t fetchLyrics(const char *artist, const char *title,
                             char *outBuffer, size_t maxLen) {
    (void)artist;
    (void)title;
    (void)outBuffer;
    (void)maxLen;
    return 0;
  }

  /**
   * @brief Notify plugin that a track was played to scrobble threshold.
   * @param track Scrobbled track information.
   * @param timestamp Unix epoch timestamp in seconds.
   */
  virtual void onTrackScrobbled(const PicAudioTrackInfo *track,
                                uint64_t timestamp) {
    (void)track;
    (void)timestamp;
  }

  /**
   * @brief Notify plugin that a track was indexed into the music library.
   * @param track Newly indexed track information.
   */
  virtual void onTrackIndexed(const PicAudioTrackInfo *track) { (void)track; }
};

} // namespace picaudio::plugin
