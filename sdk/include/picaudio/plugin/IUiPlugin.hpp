#pragma once

#include "Defines.hpp"

namespace picaudio::plugin {

/**
 * @brief Extension interface for plugins providing custom windows, dialogs, or
 * customizable action buttons on their preferences card.
 */
class IUiPlugin {
public:
  virtual ~IUiPlugin() = default;

  /**
   * @brief Registers host-provided UI services (navigation, toasts) with the plugin.
   * @param services Pointer to host UI services table.
   */
  virtual void setHostUiServices(const PicAudioHostUiServices *services) {
    (void)services;
  }

  /**
   * @brief Query custom action buttons that should appear on the plugin's preferences card.
   * @param outActions Caller-allocated buffer to populate with action descriptors.
   * @param maxActions Maximum capacity of outActions buffer.
   * @return Number of custom actions populated.
   */
  virtual size_t getCustomActions(PicAudioCustomAction *outActions,
                                  size_t maxActions) {
    (void)outActions;
    (void)maxActions;
    return 0;
  }

  /**
   * @brief Invoked when a user clicks one of the plugin's custom action buttons.
   * @param actionId Action identifier matching PicAudioCustomAction::id.
   * @return true if action was successfully handled, false otherwise.
   */
  virtual bool triggerAction(const char *actionId) {
    (void)actionId;
    return false;
  }

  /**
   * @brief Requests the plugin to display its standalone popup window or dialog.
   * @return true if window was displayed, false if unsupported or failed.
   */
  virtual bool showWindow() { return false; }

  /**
   * @brief Requests the plugin to hide its standalone popup window.
   * @return true if window was hidden, false if unsupported.
   */
  virtual bool hideWindow() { return false; }

  /**
   * @brief Queries whether the plugin's popup window is currently visible.
   */
  [[nodiscard]] virtual bool isWindowVisible() const { return false; }
};

} // namespace picaudio::plugin
