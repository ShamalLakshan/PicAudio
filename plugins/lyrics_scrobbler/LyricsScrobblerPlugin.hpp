#pragma once

#include <picaudio/plugin/Defines.hpp>
#include <picaudio/plugin/ILibraryPlugin.hpp>
#include <picaudio/plugin/IPlayerPlugin.hpp>
#include <picaudio/plugin/IPlugin.hpp>
#include <picaudio/plugin/IUiPlugin.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace picaudio::plugins {

/**
 * @brief Reference synchronized lyrics parser and scrobbler plugin.
 */
class LyricsScrobblerPlugin final : public plugin::IPicAudioPlugin,
                                    public plugin::ILibraryPlugin,
                                    public plugin::IPlayerPlugin,
                                    public plugin::IUiPlugin {
public:
  LyricsScrobblerPlugin();
  ~LyricsScrobblerPlugin() override = default;

  // IPicAudioPlugin methods
  [[nodiscard]] const char *getId() const noexcept override;
  [[nodiscard]] const char *getName() const noexcept override;
  [[nodiscard]] const char *getVersion() const noexcept override;
  [[nodiscard]] const char *getAuthor() const noexcept override;
  [[nodiscard]] const char *getDescription() const noexcept override;
  [[nodiscard]] uint32_t getApiVersion() const noexcept override;
  [[nodiscard]] uint32_t getCapabilities() const noexcept override;

  bool initialize() override;
  void start() override;
  void stop() override;
  void shutdown() override;

  [[nodiscard]] void *getCapabilityInterface(uint32_t capability) override;

  // ILibraryPlugin methods
  bool enrichMetadata(const plugin::PicAudioTrackInfo *inTrack,
                      plugin::PicAudioTrackInfo *outEnriched) override;
  size_t fetchLyrics(const char *artist, const char *title, char *outBuffer,
                     size_t maxLen) override;
  void onTrackScrobbled(const plugin::PicAudioTrackInfo *track,
                        uint64_t timestamp) override;
  void onTrackIndexed(const plugin::PicAudioTrackInfo *track) override;

  // IPlayerPlugin methods
  void onTrackStarted(const plugin::PicAudioTrackInfo *track) override;
  void onPlaybackStateChanged(plugin::PicAudioPlaybackState state) override;

  // IUiPlugin methods
  void
  setHostUiServices(const plugin::PicAudioHostUiServices *services) override;
  size_t getCustomActions(plugin::PicAudioCustomAction *outActions,
                          size_t maxActions) override;
  bool triggerAction(const char *actionId) override;
  bool showWindow() override;
  bool hideWindow() override;
  [[nodiscard]] bool isWindowVisible() const override;

private:
  std::string findSidecarLrcFile(const std::string &audioPath) const;
  std::string parseLrcContent(const std::string &lrcPath) const;

  const plugin::PicAudioHostUiServices *m_hostUiServices{nullptr};
  bool m_windowVisible{false};
  mutable std::mutex m_mutex;
  std::string m_currentFilePath;
  std::string m_currentTitle;
  std::string m_currentArtist;
  std::string m_currentAlbum;
  uint64_t m_currentDurationMs{0};
  plugin::PicAudioPlaybackState m_playbackState{
      plugin::PicAudioPlaybackState::Stopped};
  std::vector<std::string> m_scrobbleQueue;
};

} // namespace picaudio::plugins
