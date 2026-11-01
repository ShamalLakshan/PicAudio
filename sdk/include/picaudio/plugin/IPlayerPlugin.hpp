#pragma once

#include "Defines.hpp"

namespace picaudio::plugin {

/**
 * @brief Extension interface for playback events and real-time DSP filters.
 */
class IPlayerPlugin {
public:
  virtual ~IPlayerPlugin() = default;

  /**
   * @brief PCM audio buffer callback prior to host DSP processing.
   * @param buffer Mutable PCM float buffer.
   * @param frames Number of frames in buffer.
   * @param sampleRate Sampling rate in Hz.
   * @param channels Channel count.
   */
  virtual void onPreDsp(float *buffer, size_t frames, uint32_t sampleRate,
                        uint8_t channels) {
    (void)buffer;
    (void)frames;
    (void)sampleRate;
    (void)channels;
  }

  /**
   * @brief PCM audio buffer callback following host DSP processing.
   * @param buffer Mutable PCM float buffer.
   * @param frames Number of frames in buffer.
   * @param sampleRate Sampling rate in Hz.
   * @param channels Channel count.
   */
  virtual void onPostDsp(float *buffer, size_t frames, uint32_t sampleRate,
                         uint8_t channels) {
    (void)buffer;
    (void)frames;
    (void)sampleRate;
    (void)channels;
  }

  /**
   * @brief Callback invoked when playback transitions to a new track.
   * @param track Information about the starting track.
   */
  virtual void onTrackStarted(const PicAudioTrackInfo *track) { (void)track; }

  /**
   * @brief Callback invoked when playback state transitions.
   * @param state New playback state.
   */
  virtual void onPlaybackStateChanged(PicAudioPlaybackState state) {
    (void)state;
  }
};

} // namespace picaudio::plugin
