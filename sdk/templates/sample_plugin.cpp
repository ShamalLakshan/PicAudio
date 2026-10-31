#include <iostream>
#include <picaudio/plugin/Defines.hpp>
#include <picaudio/plugin/IPlugin.hpp>

namespace {

class SamplePlugin final : public picaudio::plugin::IPicAudioPlugin {
public:
  [[nodiscard]] const char *getId() const noexcept override {
    return "com.example.sample_plugin";
  }

  [[nodiscard]] const char *getName() const noexcept override {
    return "Sample Plugin";
  }

  [[nodiscard]] const char *getVersion() const noexcept override {
    return "1.0.0";
  }

  [[nodiscard]] const char *getAuthor() const noexcept override {
    return "Third-Party Developer";
  }

  [[nodiscard]] const char *getDescription() const noexcept override {
    return "Minimal reference template demonstrating PicAudio C-ABI dynamic "
           "plugin integration.";
  }

  [[nodiscard]] uint32_t getCapabilities() const noexcept override {
    return picaudio::plugin::PICAUDIO_CAP_NONE;
  }

  bool initialize() override {
    std::cout << "[SamplePlugin] Initialized successfully.\n";
    return true;
  }

  void shutdown() override {
    std::cout << "[SamplePlugin] Shutdown completed.\n";
  }
};

} // namespace

extern "C" {

PICAUDIO_PLUGIN_EXPORT picaudio::plugin::IPicAudioPlugin *create_plugin() {
  return new (std::nothrow) SamplePlugin();
}

PICAUDIO_PLUGIN_EXPORT void
destroy_plugin(picaudio::plugin::IPicAudioPlugin *plugin) {
  delete plugin;
}
}
