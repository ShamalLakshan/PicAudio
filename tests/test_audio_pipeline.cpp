#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <numeric>
#include <thread>
#include <vector>

#include "audio/AudioThreadPriority.hpp"
#include "audio/BitPerfectAudioAdapter.hpp"
#include "audio/CueSheetParser.hpp"
#include "audio/LockFreeRingBuffer.hpp"
#include "audio/WaveformExtractor.hpp"
#include "audio/alsa/AlsaAudioSink.hpp"
#include "audio/coreaudio/CoreAudioSink.hpp"
#include "audio/decoders/AudioDecoderFactory.hpp"
#include "audio/decoders/DsdAudioDecoder.hpp"
#include "audio/decoders/FlacAudioDecoder.hpp"
#include "audio/decoders/WavAudioDecoder.hpp"
#include "audio/dsp/ChannelMatrixProcessor.hpp"
#include "audio/dsp/CrossfadeEngine.hpp"
#include "audio/dsp/DspManager.hpp"
#include "audio/dsp/LoudnessAndDitherProcessor.hpp"
#include "audio/dsp/ParametricEqualizer.hpp"
#include "audio/pipewire/PipeWireAudioSink.hpp"
#include "audio/wasapi/WasapiAudioSink.hpp"
#include "core/services/PlayerService.hpp"
#include <QGuiApplication>

using namespace adapters;

static void ensureTestCoreApp() {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (!QCoreApplication::instance()) {
    static int argc = 1;
    static char appName[] = "picaudio_tests";
    static char *argv[] = {appName, nullptr};
    new QGuiApplication(argc, argv);
  }
}

TEST(AudioPipelineTest, RingBufferCapacityRounding) {
  LockFreeRingBuffer<float> rb1(100);
  EXPECT_GE(rb1.capacity(), 100u);
  // Capacity should be power of 2 minus 1
  EXPECT_EQ(rb1.capacity(), 127u);

  LockFreeRingBuffer<uint8_t> rb2(1024);
  EXPECT_EQ(rb2.capacity(), 1023u);
}

TEST(AudioPipelineTest, RingBufferBasicReadWrite) {
  LockFreeRingBuffer<int> rb(64);
  EXPECT_EQ(rb.availableRead(), 0u);
  EXPECT_EQ(rb.availableWrite(), 63u);
  EXPECT_FLOAT_EQ(rb.fillPercentage(), 0.0f);

  std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8};
  size_t written = rb.write(input.data(), input.size());
  EXPECT_EQ(written, 8u);
  EXPECT_EQ(rb.availableRead(), 8u);
  EXPECT_EQ(rb.availableWrite(), 63u - 8u);

  std::vector<int> output(8, 0);
  size_t read = rb.read(output.data(), output.size());
  EXPECT_EQ(read, 8u);
  EXPECT_EQ(output, input);
  EXPECT_EQ(rb.availableRead(), 0u);
  EXPECT_EQ(rb.availableWrite(), 63u);
}

TEST(AudioPipelineTest, RingBufferWrapAroundSemantics) {
  LockFreeRingBuffer<int> rb(64); // capacity 63

  // Write and read 50 items
  std::vector<int> firstBatch(50);
  std::iota(firstBatch.begin(), firstBatch.end(), 1);
  EXPECT_EQ(rb.write(firstBatch.data(), 50), 50u);

  std::vector<int> firstRead(50, 0);
  EXPECT_EQ(rb.read(firstRead.data(), 50), 50u);
  EXPECT_EQ(firstRead, firstBatch);

  // Now write 40 items which will wrap around the end of the buffer
  std::vector<int> secondBatch(40);
  std::iota(secondBatch.begin(), secondBatch.end(), 100);
  EXPECT_EQ(rb.write(secondBatch.data(), 40), 40u);
  EXPECT_EQ(rb.availableRead(), 40u);

  std::vector<int> secondRead(40, 0);
  EXPECT_EQ(rb.read(secondRead.data(), 40), 40u);
  EXPECT_EQ(secondRead, secondBatch);
}

TEST(AudioPipelineTest, RingBufferOverflowRejection) {
  LockFreeRingBuffer<int> rb(64); // capacity 63
  std::vector<int> data(100, 42);

  size_t written = rb.write(data.data(), data.size());
  EXPECT_EQ(written, 63u); // Cannot exceed capacity
  EXPECT_EQ(rb.availableWrite(), 0u);
  EXPECT_FLOAT_EQ(rb.fillPercentage(), 100.0f);

  // Subsequent write when full returns 0
  size_t nextWritten = rb.write(data.data(), 10);
  EXPECT_EQ(nextWritten, 0u);

  rb.reset();
  EXPECT_EQ(rb.availableRead(), 0u);
  EXPECT_EQ(rb.availableWrite(), 63u);
  EXPECT_FLOAT_EQ(rb.fillPercentage(), 0.0f);
}

TEST(AudioPipelineTest, RingBufferConcurrentSPSC) {
  constexpr size_t totalItems = 200000;
  LockFreeRingBuffer<uint32_t> rb(1024);

  std::atomic<bool> producerDone{false};
  std::vector<uint32_t> received;
  received.reserve(totalItems);

  // Producer thread
  std::thread producer([&]() {
    for (uint32_t i = 1; i <= totalItems;) {
      uint32_t chunk[32];
      size_t count = std::min<size_t>(32, totalItems - i + 1);
      for (size_t c = 0; c < count; ++c) {
        chunk[c] = i + static_cast<uint32_t>(c);
      }
      size_t written = rb.write(chunk, count);
      i += static_cast<uint32_t>(written);
      if (written == 0) {
        std::this_thread::yield();
      }
    }
    producerDone.store(true);
  });

  // Consumer thread
  std::thread consumer([&]() {
    while (!producerDone.load() || rb.availableRead() > 0) {
      uint32_t buffer[64];
      size_t readCount = rb.read(buffer, 64);
      for (size_t r = 0; r < readCount; ++r) {
        received.push_back(buffer[r]);
      }
      if (readCount == 0) {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  ASSERT_EQ(received.size(), totalItems);
  for (size_t i = 0; i < totalItems; ++i) {
    ASSERT_EQ(received[i], static_cast<uint32_t>(i + 1));
  }
}

TEST(AudioPipelineTest, RingBufferConcurrentResetSafety) {
  LockFreeRingBuffer<float> rb(2048);
  std::atomic<bool> running{true};

  std::thread writer([&]() {
    std::vector<float> data(64, 1.0f);
    while (running.load(std::memory_order_relaxed)) {
      rb.write(data.data(), data.size());
      std::this_thread::yield();
    }
  });

  std::thread reader([&]() {
    std::vector<float> buf(64, 0.0f);
    while (running.load(std::memory_order_relaxed)) {
      rb.read(buf.data(), buf.size());
      std::this_thread::yield();
    }
  });

  for (int i = 0; i < 500; ++i) {
    rb.reset();
    size_t availRead = rb.availableRead();
    size_t availWrite = rb.availableWrite();
    EXPECT_LE(availRead, rb.capacity());
    EXPECT_LE(availWrite, rb.capacity());
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  running.store(false, std::memory_order_relaxed);
  writer.join();
  reader.join();

  EXPECT_LE(rb.availableRead(), rb.capacity());
  EXPECT_LE(rb.availableWrite(), rb.capacity());
}

TEST(AudioPipelineTest, AudioThreadPriorityConfiguration) {
  // Query initial priority
  auto initial = getCurrentAudioThreadPriority();
  EXPECT_FALSE(initial.message.empty());

  // Configure round-robin real-time priority (graceful fallback check)
  auto rrResult =
      configureAudioThreadPriority(AudioPriorityMode::RealtimeRoundRobin, 50);
  EXPECT_FALSE(rrResult.message.empty());

  // Configure normal priority
  auto normalResult = configureAudioThreadPriority(AudioPriorityMode::Normal);
  EXPECT_FALSE(normalResult.message.empty());
  EXPECT_EQ(normalResult.activeMode, AudioPriorityMode::Normal);
}

TEST(AudioPipelineTest, AlsaAudioSinkEnumerationAndProperties) {
  AlsaAudioSink sink;
  EXPECT_EQ(sink.getName(), "ALSA Direct Hardware (hw:)");
  EXPECT_FALSE(sink.isOpen());
  EXPECT_TRUE(sink.isBitPerfect());
  EXPECT_EQ(sink.getXrunCount(), 0u);

  auto devices = sink.enumerateDevices();
  ASSERT_FALSE(devices.empty());
  EXPECT_EQ(devices[0].id, "default");
  EXPECT_TRUE(devices[0].isDefault);
}

TEST(AudioPipelineTest, PipeWireAudioSinkEnumerationAndProperties) {
  PipeWireAudioSink sink;
  EXPECT_EQ(sink.getName(), "Linux PipeWire Lock-Free Client");
  EXPECT_FALSE(sink.isOpen());
  EXPECT_TRUE(sink.isBitPerfect());
  EXPECT_EQ(sink.getXrunCount(), 0u);

  auto devices = sink.enumerateDevices();
  ASSERT_FALSE(devices.empty());
  EXPECT_EQ(devices[0].id, "pipewire-default");
  EXPECT_TRUE(devices[0].isDefault);
}

TEST(AudioPipelineTest, BitPerfectAdapterBackendRouting) {
  ensureTestCoreApp();
  auto adapter = std::make_shared<BitPerfectAudioAdapter>();
  EXPECT_TRUE(adapter->initialize(96000, 2));

  adapter->setAudioBackend("ALSA Direct Hardware (hw:)");
  EXPECT_EQ(adapter->getAudioBackend(), "ALSA Direct Hardware (hw:)");

  auto alsaDevices = adapter->getAvailableDevices();
  EXPECT_FALSE(alsaDevices.empty());

  adapter->setAudioBackend("Linux PipeWire Lock-Free Client");
  EXPECT_EQ(adapter->getAudioBackend(), "Linux PipeWire Lock-Free Client");

  auto pwDevices = adapter->getAvailableDevices();
  EXPECT_FALSE(pwDevices.empty());

  adapter->setBufferLatencyMs(25);
  EXPECT_EQ(adapter->getBufferLatencyMs(), 25u);

  adapter->setBitPerfectExclusive(true);
  EXPECT_TRUE(adapter->isBitPerfectExclusive());

  core::PlayerService player(adapter);
  EXPECT_EQ(player.getAudioBackend(), "Linux PipeWire Lock-Free Client");
  player.setAudioBackend("ALSA Direct Hardware (hw:)");
  EXPECT_EQ(player.getAudioBackend(), "ALSA Direct Hardware (hw:)");
  EXPECT_EQ(player.getBufferLatencyMs(), 25u);
  player.setBufferLatencyMs(40);
  EXPECT_EQ(player.getBufferLatencyMs(), 40u);
}

TEST(AudioPipelineTest, ChannelMatrixProcessorIdentityAndConfig) {
  ChannelMatrixProcessor proc;
  proc.configure(2, 2);
  EXPECT_EQ(proc.getInputChannels(), 2);
  EXPECT_EQ(proc.getOutputChannels(), 2);
  EXPECT_EQ(proc.getActivePreset(), MatrixPreset::Passthrough);

  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 0), 1.0f);
  EXPECT_FLOAT_EQ(proc.getCoefficient(1, 1), 1.0f);
  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 1), 0.0f);
  EXPECT_FLOAT_EQ(proc.getCoefficient(1, 0), 0.0f);

  proc.setCoefficient(0, 1, 0.5f);
  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 1), 0.5f);
  EXPECT_EQ(proc.getActivePreset(), MatrixPreset::Custom);

  proc.resetToIdentity();
  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 1), 0.0f);
  EXPECT_EQ(proc.getActivePreset(), MatrixPreset::Passthrough);
}

TEST(AudioPipelineTest, ChannelMatrixProcessorDownmixToStereo) {
  ChannelMatrixProcessor proc;
  proc.configure(6, 2);
  proc.loadPreset(MatrixPreset::DownmixToStereo);
  EXPECT_EQ(proc.getActivePreset(), MatrixPreset::DownmixToStereo);

  // ITU-R BS.775 normalized coefficients (prevents clipping when summing)
  EXPECT_NEAR(proc.getCoefficient(0, 0), 0.4142f, 0.01f);
  EXPECT_NEAR(proc.getCoefficient(1, 1), 0.4142f, 0.01f);

  // FC (index 2) mixed into L and R at ~0.2929f
  EXPECT_NEAR(proc.getCoefficient(0, 2), 0.2929f, 0.01f);
  EXPECT_NEAR(proc.getCoefficient(1, 2), 0.2929f, 0.01f);

  // RL and RR (indices 4, 5) mixed into L and R at ~0.2929f
  EXPECT_NEAR(proc.getCoefficient(0, 4), 0.2929f, 0.01f);
  EXPECT_NEAR(proc.getCoefficient(1, 5), 0.2929f, 0.01f);

  // Test processing audio frame with only Center channel active: 1.0f
  std::vector<float> input = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
  std::vector<float> output(2, 0.0f);
  proc.process(input.data(), output.data(), 1);

  EXPECT_NEAR(output[0], 0.2929f, 0.02f);
  EXPECT_NEAR(output[1], 0.2929f, 0.02f);
}

TEST(AudioPipelineTest, ChannelMatrixProcessorDownmixToMono) {
  ChannelMatrixProcessor proc;
  proc.configure(2, 1);
  proc.loadPreset(MatrixPreset::DownmixToMono);
  EXPECT_EQ(proc.getActivePreset(), MatrixPreset::DownmixToMono);

  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 0), 0.5f);
  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 1), 0.5f);

  std::vector<float> input = {0.8f, 0.4f};
  std::vector<float> output(1, 0.0f);
  proc.process(input.data(), output.data(), 1);

  // (0.8 * 0.5) + (0.4 * 0.5) = 0.6f
  EXPECT_NEAR(output[0], 0.6f, 0.02f);
}

TEST(AudioPipelineTest, ChannelMatrixProcessorUpmixTo51And71) {
  ChannelMatrixProcessor proc51;
  proc51.configure(2, 6);
  proc51.loadPreset(MatrixPreset::UpmixTo51);
  EXPECT_EQ(proc51.getActivePreset(), MatrixPreset::UpmixTo51);

  // Front Left and Front Right
  EXPECT_FLOAT_EQ(proc51.getCoefficient(0, 0), 1.0f);
  EXPECT_FLOAT_EQ(proc51.getCoefficient(1, 1), 1.0f);
  // Center (FC: index 2 derived from FL+FR / sqrt(2) * 0.7)
  EXPECT_NEAR(proc51.getCoefficient(2, 0), 0.495f, 0.01f);
  EXPECT_NEAR(proc51.getCoefficient(2, 1), 0.495f, 0.01f);
  // LFE (Sub: index 3)
  EXPECT_FLOAT_EQ(proc51.getCoefficient(3, 0), 0.5f);
  EXPECT_FLOAT_EQ(proc51.getCoefficient(3, 1), 0.5f);
  // Surround/Rear (RL/RR: index 4, 5)
  EXPECT_NEAR(proc51.getCoefficient(4, 0), 0.495f, 0.01f);
  EXPECT_NEAR(proc51.getCoefficient(5, 1), 0.495f, 0.01f);

  // Test processing stereo input into 6 channels
  std::vector<float> input = {0.6f, 0.6f};
  std::vector<float> output51(6, 0.0f);
  proc51.process(input.data(), output51.data(), 1);

  EXPECT_GT(output51[0], 0.0f); // FL
  EXPECT_GT(output51[1], 0.0f); // FR
  EXPECT_GT(output51[2], 0.0f); // Center
  EXPECT_GT(output51[3], 0.0f); // Subwoofer
  EXPECT_GT(output51[4], 0.0f); // RL
  EXPECT_GT(output51[5], 0.0f); // RR

  ChannelMatrixProcessor proc71;
  proc71.configure(2, 8);
  proc71.loadPreset(MatrixPreset::UpmixTo71);
  EXPECT_EQ(proc71.getActivePreset(), MatrixPreset::UpmixTo71);
  // Side speakers (index 6, 7)
  EXPECT_NEAR(proc71.getCoefficient(6, 0), 0.65f, 0.01f);
  EXPECT_NEAR(proc71.getCoefficient(7, 1), 0.65f, 0.01f);

  std::vector<float> output71(8, 0.0f);
  proc71.process(input.data(), output71.data(), 1);
  for (size_t ch = 0; ch < 8; ++ch) {
    EXPECT_GT(output71[ch], 0.0f);
  }
}

TEST(AudioPipelineTest, ChannelMatrixProcessorNormalizationAndLimiter) {
  ChannelMatrixProcessor proc;
  proc.configure(2, 2);
  // Intentionally set high gain
  proc.setCoefficient(0, 0, 1.5f);
  proc.setCoefficient(0, 1, 1.5f);
  EXPECT_FLOAT_EQ(proc.getCoefficient(0, 0), 1.5f);

  proc.normalize();
  float sum = proc.getCoefficient(0, 0) + proc.getCoefficient(0, 1);
  EXPECT_LE(sum, 1.0001f);
  EXPECT_NEAR(proc.getCoefficient(0, 0), 0.5f, 0.01f);
  EXPECT_NEAR(proc.getCoefficient(0, 1), 0.5f, 0.01f);

  // Soft-knee limiter test: massive input signal
  proc.setCoefficient(0, 0, 4.0f);
  std::vector<float> input = {10.0f, 0.0f};
  std::vector<float> output(2, 0.0f);
  proc.process(input.data(), output.data(), 1);
  // Output should be limited <= 1.0f without NaN or Inf
  EXPECT_FALSE(std::isnan(output[0]));
  EXPECT_FALSE(std::isinf(output[0]));
  EXPECT_LE(output[0], 1.0f);
}

TEST(AudioPipelineTest, ChannelMatrixProcessorSerialization) {
  ChannelMatrixProcessor proc1;
  proc1.configure(6, 2);
  proc1.loadPreset(MatrixPreset::DownmixToStereo);
  proc1.setCoefficient(0, 2, 0.85f);

  std::string json = proc1.serialize();
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("\"inChannels\":6"), std::string::npos);
  EXPECT_NE(json.find("\"outChannels\":2"), std::string::npos);

  ChannelMatrixProcessor proc2;
  EXPECT_TRUE(proc2.deserialize(json));
  EXPECT_EQ(proc2.getInputChannels(), 6);
  EXPECT_EQ(proc2.getOutputChannels(), 2);
  EXPECT_NEAR(proc2.getCoefficient(0, 2), 0.85f, 0.001f);
}

TEST(AudioPipelineTest, BitPerfectEngineChannelMatrixIntegration) {
  ensureTestCoreApp();
  auto engine = std::make_shared<BitPerfectAudioEngine>();
  EXPECT_TRUE(engine->initialize(48000, 2));

  // Default output channel count
  EXPECT_EQ(engine->getOutputChannelCount(), 2);

  // Switch output channel count to 6 (5.1)
  engine->setOutputChannelCount(6);
  EXPECT_EQ(engine->getOutputChannelCount(), 6);

  // Set preset
  engine->setChannelMatrixPreset(3); // UpmixTo51
  EXPECT_EQ(engine->getChannelMatrixPreset(), 3);
  EXPECT_NEAR(engine->getChannelMatrixCoefficient(2, 0), 0.495f, 0.01f);

  // Custom modification
  engine->setChannelMatrixCoefficient(2, 0, 0.75f);
  EXPECT_FLOAT_EQ(engine->getChannelMatrixCoefficient(2, 0), 0.75f);

  engine->normalizeChannelMatrix();

  auto levels = engine->getChannelLevels();
  EXPECT_EQ(levels.size(), 8u);

  std::string serialized = engine->getSerializedChannelMatrix();
  EXPECT_FALSE(serialized.empty());
  EXPECT_TRUE(engine->setSerializedChannelMatrix(serialized));

  // PlayerService integration
  core::PlayerService player(engine);
  EXPECT_EQ(player.getOutputChannelCount(), 6);
  player.setOutputChannelCount(8);
  EXPECT_EQ(player.getOutputChannelCount(), 8);
  player.setChannelMatrixPreset(4); // UpmixTo71
  EXPECT_EQ(player.getChannelMatrixPreset(), 4);
}

TEST(AudioPipelineTest, ParametricEqualizerBasicConfigAndBands) {
  ParametricEqualizer eq;
  eq.configure(48000, 2);
  EXPECT_FALSE(eq.isBypassed());
  EXPECT_FLOAT_EQ(eq.getPreampGain(), 0.0f);
  EXPECT_EQ(eq.getPreset(), "Flat");

  auto initialGains = eq.getBandGains();
  EXPECT_EQ(initialGains.size(), 10u);
  for (float g : initialGains) {
    EXPECT_FLOAT_EQ(g, 0.0f);
  }

  eq.setBandGain(0, 4.5f);
  EXPECT_FLOAT_EQ(eq.getBandGain(0), 4.5f);

  eq.setBandGain(9, -6.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(9), -6.0f);

  // Out of range clamping
  eq.setBandGain(3, 20.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(3), 12.0f);

  eq.setBandGain(3, -25.0f);
  EXPECT_FLOAT_EQ(eq.getBandGain(3), -12.0f);

  eq.setPreampGain(3.0f);
  EXPECT_FLOAT_EQ(eq.getPreampGain(), 3.0f);
}

TEST(AudioPipelineTest, ParametricEqualizerBypassIdentity) {
  ParametricEqualizer eq;
  eq.configure(44100, 2);

  // Configure high boosts
  eq.setBandGain(0, 10.0f);
  eq.setBandGain(5, -8.0f);
  eq.setPreampGain(5.0f);

  // When bypassed, input buffer is untouched bit-for-bit
  eq.setBypassed(true);
  EXPECT_TRUE(eq.isBypassed());

  std::vector<float> buffer = {0.1f, -0.2f, 0.5f, -0.4f, 0.8f, -0.7f};
  std::vector<float> original = buffer;

  eq.process(buffer.data(), 3);
  EXPECT_EQ(buffer, original);

  // Now enable and verify it modifies audio
  eq.setBypassed(false);
  eq.process(buffer.data(), 3);
  EXPECT_NE(buffer, original);
}

TEST(AudioPipelineTest, ParametricEqualizerPresetAndSerialization) {
  ParametricEqualizer eq;
  eq.configure(96000, 2);

  eq.setPreset("Bass Boost");
  EXPECT_EQ(eq.getPreset(), "Bass Boost");
  EXPECT_GT(eq.getBandGain(0), 4.0f);

  eq.setPreampGain(-2.0f);

  std::string json = eq.serialize();
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("\"Bass Boost\""), std::string::npos);

  ParametricEqualizer restored;
  restored.configure(96000, 2);
  EXPECT_TRUE(restored.deserialize(json));
  EXPECT_EQ(restored.getPreset(), "Bass Boost");
  EXPECT_FLOAT_EQ(restored.getPreampGain(), -2.0f);
  EXPECT_FLOAT_EQ(restored.getBandGain(0), eq.getBandGain(0));
}

TEST(AudioPipelineTest, ParametricEqualizerMultiChannelAndClipping) {
  ParametricEqualizer eq;
  eq.configure(48000, 6); // 5.1 surround
  eq.setPreampGain(12.0f);
  for (size_t i = 0; i < 10; ++i) {
    eq.setBandGain(i, 12.0f);
  }

  // Large input signals to test soft limiting
  std::vector<float> buffer(60, 0.9f);
  eq.process(buffer.data(), 10);

  // Check no samples exploded or resulted in NaN/inf
  for (float s : buffer) {
    EXPECT_FALSE(std::isnan(s));
    EXPECT_FALSE(std::isinf(s));
    EXPECT_LE(s, 1.0f);
    EXPECT_GE(s, -1.0f);
  }
}

TEST(AudioPipelineTest, DspManagerInitializationAndDirectories) {
  ensureTestCoreApp();
  DspManager dsp;
  EXPECT_TRUE(dsp.isDspMasterEnabled());
  EXPECT_EQ(dsp.getRoutingOrder(), DspManager::RoutingOrder::EqThenDsp);

  std::string primaryDir = dsp.getPrimaryDspDirectory();
  EXPECT_FALSE(primaryDir.empty());

  // Verify directory and README.txt exist
  EXPECT_TRUE(std::filesystem::exists(primaryDir));
  EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(primaryDir) /
                                      "README.txt"));

  // Check routing order change
  dsp.setRoutingOrder(DspManager::RoutingOrder::DspThenEq);
  EXPECT_EQ(dsp.getRoutingOrder(), DspManager::RoutingOrder::DspThenEq);

  dsp.setDspMasterEnabled(false);
  EXPECT_FALSE(dsp.isDspMasterEnabled());

  // Test setAllPluginsEnabled invocation without plugins does not crash
  dsp.setAllPluginsEnabled(true);
  dsp.setAllPluginsEnabled(false);
}

TEST(AudioPipelineTest, ParametricEqualizerDynamicBandCounts) {
  ParametricEqualizer eq;
  eq.configure(48000, 2);

  // Default is 10 bands
  EXPECT_EQ(eq.getBandCount(), 10u);
  EXPECT_EQ(eq.getBandGains().size(), 10u);
  EXPECT_EQ(eq.getBandFrequencies().size(), 10u);
  EXPECT_EQ(eq.getBandFrequencyLabels().size(), 10u);
  EXPECT_EQ(eq.getBandFrequencyLabels()[0], "31 Hz");
  EXPECT_EQ(eq.getBandFrequencyLabels()[9], "16 kHz");

  // Switch to 15 bands
  eq.setBandCount(15);
  EXPECT_EQ(eq.getBandCount(), 15u);
  EXPECT_EQ(eq.getBandGains().size(), 15u);
  EXPECT_EQ(eq.getBandFrequencies().size(), 15u);
  EXPECT_EQ(eq.getBandFrequencyLabels().size(), 15u);
  EXPECT_EQ(eq.getBandFrequencyLabels()[0], "25 Hz");
  EXPECT_EQ(eq.getBandFrequencyLabels()[14], "16 kHz");

  // Switch to 31 bands
  eq.setBandCount(31);
  EXPECT_EQ(eq.getBandCount(), 31u);
  EXPECT_EQ(eq.getBandGains().size(), 31u);
  EXPECT_EQ(eq.getBandFrequencies().size(), 31u);
  EXPECT_EQ(eq.getBandFrequencyLabels().size(), 31u);
  EXPECT_EQ(eq.getBandFrequencyLabels()[0], "20");
  EXPECT_EQ(eq.getBandFrequencyLabels()[30], "20k");

  // Setting preset interpolates smoothly across 31 bands
  eq.setPreset("Bass Boost");
  EXPECT_EQ(eq.getPreset(), "Bass Boost");
  EXPECT_GT(eq.getBandGain(0), 4.0f);
  EXPECT_GT(eq.getBandGain(5), 1.0f);

  // Test audio processing across 31 bands
  std::vector<float> buffer = {0.2f, -0.3f, 0.4f, -0.1f, 0.5f, -0.6f};
  std::vector<float> original = buffer;
  eq.process(buffer.data(), 3);
  EXPECT_NE(buffer, original);

  // Switch back to 10 bands
  eq.setBandCount(10);
  EXPECT_EQ(eq.getBandCount(), 10u);
  EXPECT_EQ(eq.getBandGains().size(), 10u);
}

TEST(AudioPipelineTest, ParametricEqualizerConcurrentThreadStressTest) {
  ParametricEqualizer eq;
  eq.configure(48000, 2);

  std::atomic<bool> running{true};
  std::atomic<uint64_t> processedBlocks{0};

  // Audio thread simulating real-time audio playback
  std::thread audioThread([&]() {
    std::vector<float> buffer(512 * 2, 0.5f);
    while (running.load(std::memory_order_relaxed)) {
      eq.process(buffer.data(), 512);
      processedBlocks.fetch_add(1, std::memory_order_relaxed);
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });

  // UI thread rapidly changing bands, presets, preamp, and bypass state
  for (int i = 0; i < 200; ++i) {
    eq.setBandGain(i % 10, static_cast<float>((i % 24) - 12));
    eq.setPreampGain(static_cast<float>((i % 10) - 5));
    if (i % 20 == 0) {
      eq.setBypassed((i / 20) % 2 == 1);
    }
    if (i % 30 == 0) {
      eq.setPreset(i % 60 == 0 ? "Rock" : "Flat");
    }
    if (i % 50 == 0) {
      eq.setBandCount(i % 100 == 0 ? 15 : 10);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(200));
  }

  running.store(false, std::memory_order_release);
  if (audioThread.joinable()) {
    audioThread.join();
  }

  EXPECT_GT(processedBlocks.load(), 50u);
}

TEST(AudioPipelineTest, AudioEngineAndPlayerServiceEqDspRouting) {
  ensureTestCoreApp();
  auto engine = std::make_shared<BitPerfectAudioEngine>();
  engine->initialize(44100, 2);

  core::PlayerService player(engine);

  // Equalizer routing through PlayerService
  EXPECT_TRUE(player.isEqEnabled());
  player.setEqEnabled(false);
  EXPECT_FALSE(player.isEqEnabled());
  player.setEqEnabled(true);
  EXPECT_TRUE(player.isEqEnabled());

  player.setEqPreampGain(2.5f);
  EXPECT_FLOAT_EQ(player.getEqPreampGain(), 2.5f);

  player.setEqBandGain(1, 3.5f);
  EXPECT_FLOAT_EQ(player.getEqBandGain(1), 3.5f);

  player.setEqPreset("Rock");
  EXPECT_EQ(player.getEqPreset(), "Rock");

  // Dynamic band count through PlayerService
  EXPECT_EQ(player.getEqBandCount(), 10u);
  EXPECT_EQ(player.getEqBandFrequencyLabels().size(), 10u);

  player.setEqBandCount(15);
  EXPECT_EQ(player.getEqBandCount(), 15u);
  EXPECT_EQ(player.getEqBandFrequencyLabels().size(), 15u);

  player.setEqBandCount(31);
  EXPECT_EQ(player.getEqBandCount(), 31u);
  EXPECT_EQ(player.getEqBandFrequencyLabels().size(), 31u);

  player.resetEq();
  EXPECT_FLOAT_EQ(player.getEqPreampGain(), 0.0f);
  EXPECT_FLOAT_EQ(player.getEqBandGain(1), 0.0f);

  // DSP Routing order through PlayerService
  EXPECT_EQ(player.getDspRoutingOrder(), 0);
  player.setDspRoutingOrder(1);
  EXPECT_EQ(player.getDspRoutingOrder(), 1);

  EXPECT_TRUE(player.isDspMasterEnabled());
  player.setDspMasterEnabled(false);
  EXPECT_FALSE(player.isDspMasterEnabled());
}

TEST(AudioPipelineTest, WasapiAudioSinkLifecycleAndProperties) {
  WasapiAudioSink sink;
  EXPECT_FALSE(sink.isOpen());
  EXPECT_EQ(sink.getName(), "Windows WASAPI Exclusive Mode");

  auto devices = sink.enumerateDevices();
  EXPECT_FALSE(devices.empty());

  AudioSinkConfig config;
  config.sampleRate = 48000;
  config.channels = 2;
  config.bitDepth = 24;
  config.bufferLatencyMs = 15;

  EXPECT_TRUE(sink.open(config));
  EXPECT_TRUE(sink.isOpen());
  EXPECT_TRUE(sink.isBitPerfect());
  EXPECT_EQ(sink.getLatencyMs(), 15u);

  std::vector<float> pcm(512 * 2, 0.2f);
  size_t written = sink.write(pcm.data(), 512);
  EXPECT_GT(written, 0u);

  sink.pause();
  sink.resume();
  sink.flush();
  sink.close();
  EXPECT_FALSE(sink.isOpen());
}

TEST(AudioPipelineTest, CoreAudioSinkLifecycleAndHogMode) {
  CoreAudioSink sink;
  EXPECT_FALSE(sink.isOpen());
  EXPECT_EQ(sink.getName(), "macOS CoreAudio Integer/Hog Mode");

  auto devices = sink.enumerateDevices();
  EXPECT_FALSE(devices.empty());

  AudioSinkConfig config;
  config.sampleRate = 96000;
  config.channels = 2;
  config.bitDepth = 24;
  config.bufferLatencyMs = 10;

  EXPECT_TRUE(sink.open(config));
  EXPECT_TRUE(sink.isOpen());
  EXPECT_TRUE(sink.isBitPerfect());

  EXPECT_TRUE(sink.setHogMode(true));
  EXPECT_TRUE(sink.isHogModeActive());
  EXPECT_TRUE(sink.setHogMode(false));
  EXPECT_FALSE(sink.isHogModeActive());

  std::vector<float> pcm(256 * 2, 0.1f);
  size_t written = sink.write(pcm.data(), 256);
  EXPECT_EQ(written, 256u);

  sink.close();
  EXPECT_FALSE(sink.isOpen());
}

TEST(AudioPipelineTest, WavAudioDecoderDirectPcmParsing) {
  std::filesystem::path tempWav =
      std::filesystem::temp_directory_path() / "test_synth.wav";

  // Synthesize a valid 16-bit 44.1kHz stereo PCM WAV file
  std::ofstream out(tempWav, std::ios::binary);
  ASSERT_TRUE(out.is_open());

  uint32_t sampleRate = 44100;
  uint16_t channels = 2;
  uint16_t bitsPerSample = 16;
  uint32_t frameCount = 100;
  uint32_t dataBytes = frameCount * channels * (bitsPerSample / 8);
  uint32_t riffSize = 36 + dataBytes;

  out.write("RIFF", 4);
  out.write(reinterpret_cast<const char *>(&riffSize), 4);
  out.write("WAVE", 4);

  // 'fmt ' chunk
  uint32_t fmtSize = 16;
  uint16_t fmtTag = 1; // PCM
  uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  uint16_t blockAlign = channels * (bitsPerSample / 8);
  out.write("fmt ", 4);
  out.write(reinterpret_cast<const char *>(&fmtSize), 4);
  out.write(reinterpret_cast<const char *>(&fmtTag), 2);
  out.write(reinterpret_cast<const char *>(&channels), 2);
  out.write(reinterpret_cast<const char *>(&sampleRate), 4);
  out.write(reinterpret_cast<const char *>(&byteRate), 4);
  out.write(reinterpret_cast<const char *>(&blockAlign), 2);
  out.write(reinterpret_cast<const char *>(&bitsPerSample), 2);

  // 'data' chunk
  out.write("data", 4);
  out.write(reinterpret_cast<const char *>(&dataBytes), 4);
  for (uint32_t i = 0; i < frameCount * channels; ++i) {
    int16_t sample = static_cast<int16_t>((i % 100) * 300);
    out.write(reinterpret_cast<const char *>(&sample), 2);
  }
  out.close();

  WavAudioDecoder decoder;
  EXPECT_TRUE(decoder.open(tempWav.string()));
  EXPECT_TRUE(decoder.isOpen());

  auto meta = decoder.getMetadata();
  EXPECT_EQ(meta.sampleRate, 44100u);
  EXPECT_EQ(meta.channels, 2u);
  EXPECT_EQ(meta.bitDepth, 16u);
  EXPECT_EQ(meta.totalFrames, 100u);

  std::vector<float> readBuf(50 * 2);
  size_t read = decoder.readFrames(readBuf.data(), 50);
  EXPECT_EQ(read, 50u);
  EXPECT_EQ(decoder.getPositionFrames(), 50u);

  EXPECT_TRUE(decoder.seek(10));
  EXPECT_EQ(decoder.getPositionFrames(), 10u);

  decoder.close();
  EXPECT_FALSE(decoder.isOpen());
  std::filesystem::remove(tempWav);
}

TEST(AudioPipelineTest, FlacAudioDecoderStreamInfoParsing) {
  std::filesystem::path tempFlac =
      std::filesystem::temp_directory_path() / "test_synth.flac";

  // Synthesize a valid FLAC header with STREAMINFO metadata
  std::ofstream out(tempFlac, std::ios::binary);
  ASSERT_TRUE(out.is_open());

  out.write("fLaC", 4);

  // STREAMINFO metadata block header (isLast=true, type=0, length=34)
  uint8_t blockHeader[4] = {0x80, 0x00, 0x00, 34};
  out.write(reinterpret_cast<const char *>(blockHeader), 4);

  uint8_t streamInfo[34] = {0};
  // min/max blocksize: 4096
  streamInfo[0] = 0x10;
  streamInfo[1] = 0x00;
  streamInfo[2] = 0x10;
  streamInfo[3] = 0x00;

  // sample rate = 96000 (0x17700), channels = 2 (ch-1 = 1), bits = 24 (b-1 =
  // 23) 96000 = 0x17700: info[10]=0x17, info[11]=0x70, info[12]=(0x0 << 4) | (1
  // << 1) | (23 >> 4) = 0x03
  streamInfo[10] = 0x17;
  streamInfo[11] = 0x70;
  streamInfo[12] = 0x03;
  // info[13] = ((23 & 0x0F) << 4) | (0 for total samples high 4 bits) = 0x70
  streamInfo[13] = 0x70;
  // total samples = 480000 (0x075300)
  streamInfo[14] = 0x00;
  streamInfo[15] = 0x07;
  streamInfo[16] = 0x53;
  streamInfo[17] = 0x00;

  out.write(reinterpret_cast<const char *>(streamInfo), 34);
  out.close();

  FlacAudioDecoder decoder;
  EXPECT_TRUE(decoder.open(tempFlac.string()));
  EXPECT_TRUE(decoder.isOpen());

  auto meta = decoder.getMetadata();
  EXPECT_EQ(meta.sampleRate, 96000u);
  EXPECT_EQ(meta.channels, 2u);
  EXPECT_EQ(meta.bitDepth, 24u);
  EXPECT_EQ(meta.totalFrames, 480000u);

  decoder.close();
  std::filesystem::remove(tempFlac);
}

TEST(AudioPipelineTest, DsdAudioDecoderDsfParsingAndDoP) {
  std::filesystem::path tempDsf =
      std::filesystem::temp_directory_path() / "test_synth.dsf";

  std::ofstream out(tempDsf, std::ios::binary);
  ASSERT_TRUE(out.is_open());

  // 'DSD ' chunk (28 bytes)
  uint8_t dsdChunk[28] = {'D', 'S', 'D', ' '};
  *reinterpret_cast<uint64_t *>(dsdChunk + 4) = 28;
  *reinterpret_cast<uint64_t *>(dsdChunk + 12) = 1000;
  out.write(reinterpret_cast<const char *>(dsdChunk), 28);

  // 'fmt ' chunk (52 bytes)
  uint8_t fmtChunk[52] = {'f', 'm', 't', ' '};
  *reinterpret_cast<uint64_t *>(fmtChunk + 4) = 52;
  *reinterpret_cast<uint32_t *>(fmtChunk + 12) = 1;       // version
  *reinterpret_cast<uint32_t *>(fmtChunk + 16) = 0;       // format id
  *reinterpret_cast<uint32_t *>(fmtChunk + 20) = 1;       // channel type
  *reinterpret_cast<uint32_t *>(fmtChunk + 24) = 2;       // channel count
  *reinterpret_cast<uint32_t *>(fmtChunk + 28) = 2822400; // DSD64 2.8224MHz
  *reinterpret_cast<uint32_t *>(fmtChunk + 32) = 1;       // 1-bit
  *reinterpret_cast<uint64_t *>(fmtChunk + 36) = 2822400; // 1 second of DSD
  *reinterpret_cast<uint32_t *>(fmtChunk + 44) = 4096;    // block size
  out.write(reinterpret_cast<const char *>(fmtChunk), 52);

  // 'data' chunk
  uint8_t dataChunk[12] = {'d', 'a', 't', 'a'};
  *reinterpret_cast<uint64_t *>(dataChunk + 4) = 705600 + 12;
  out.write(reinterpret_cast<const char *>(dataChunk), 12);
  out.close();

  DsdAudioDecoder decoder;
  EXPECT_TRUE(decoder.open(tempDsf.string()));
  EXPECT_TRUE(decoder.isOpen());

  auto meta = decoder.getMetadata();
  EXPECT_TRUE(meta.isDsd);
  EXPECT_EQ(meta.dsdSampleRate, 2822400u);
  EXPECT_EQ(meta.sampleRate, 88200u); // Decimated rate
  EXPECT_EQ(meta.channels, 2u);

  // Test DoP (DSD over PCM) output mode
  decoder.setOutputMode(DsdAudioDecoder::OutputMode::DsdOverPcm);
  EXPECT_EQ(decoder.getOutputMode(), DsdAudioDecoder::OutputMode::DsdOverPcm);

  std::vector<float> dopBuf(64 * 2);
  size_t read = decoder.readFrames(dopBuf.data(), 64);
  EXPECT_EQ(read, 64u);
  // DoP produces marker bytes 0x05 / 0xFA
  EXPECT_NE(dopBuf[0], 0.0f);

  decoder.close();
  std::filesystem::remove(tempDsf);
}

TEST(AudioPipelineTest, LoudnessAndDitherProcessorReplayGainAndLimiter) {
  LoudnessAndDitherProcessor proc;

  // Test SmartGain mode with ReplayGain tags
  proc.setReplayGainMode(ReplayGainMode::SmartGain);
  proc.setPreampTaggedDb(3.0f);
  proc.setPreampUntaggedDb(-6.0f);
  proc.setCurrentTrackLoudness(true, -4.5f, 0.95f, -3.0f, 0.98f, true);

  // In album playback, album gain (-3.0) + preamp (+3.0) = 0.0 dB (linear 1.0)
  EXPECT_FLOAT_EQ(proc.getCurrentAppliedGainDb(), 0.0f);

  // Now switch to shuffle (track gain mode): -4.5 + 3.0 = -1.5 dB
  proc.setCurrentTrackLoudness(true, -4.5f, 0.95f, -3.0f, 0.98f, false);
  EXPECT_FLOAT_EQ(proc.getCurrentAppliedGainDb(), -1.5f);

  // Test True-Peak Anti-Clipping Limiter with high positive preamp (+10 dB)
  proc.setPreampTaggedDb(10.0f);
  proc.setTruePeakLimiterEnabled(true);
  EXPECT_TRUE(proc.isTruePeakLimiterEnabled());

  std::vector<float> audio = {0.8f, -0.9f, 0.95f, -0.99f};
  proc.process(audio.data(), 2, 2);

  // Verify True-Peak limiter prevents digital clipping past [-1.0, +1.0]
  for (float s : audio) {
    EXPECT_LE(s, 1.0f);
    EXPECT_GE(s, -1.0f);
  }

  // Test 64-bit TPDF Dithering
  proc.setDitherMode(DitherMode::FlatTpdf);
  EXPECT_EQ(proc.getDitherMode(), DitherMode::FlatTpdf);
  proc.setTargetBitDepth(16);
  EXPECT_EQ(proc.getTargetBitDepth(), 16u);

  std::vector<float> quietAudio = {0.0f, 0.0f, 0.0f, 0.0f};
  proc.process(quietAudio.data(), 2, 2);
  // Dither injects low-level triangular noise
  bool hasNoise = false;
  for (float s : quietAudio) {
    if (std::abs(s) > 0.0f)
      hasNoise = true;
  }
  EXPECT_TRUE(hasNoise);
}

TEST(AudioPipelineTest, CrossfadeEngineCurvesAndAlbumProtection) {
  CrossfadeEngine crossfader;

  crossfader.setEnabled(true);
  EXPECT_TRUE(crossfader.isEnabled());

  crossfader.setDurationSec(3.0f);
  EXPECT_FLOAT_EQ(crossfader.getDurationSec(), 3.0f);

  crossfader.setAutoDisableSameAlbum(true);
  EXPECT_TRUE(crossfader.isAutoDisableSameAlbum());

  // Consecutive tracks on same album must NOT crossfade to protect gapless
  // concept album playback
  EXPECT_FALSE(crossfader.shouldTriggerCrossfade("The Wall", "The Wall", 2000));

  // Mixed tracks on different albums MUST trigger crossfade
  EXPECT_TRUE(
      crossfader.shouldTriggerCrossfade("Abbey Road", "The Wall", 2000));

  // Test Equal-Power crossfade blending
  crossfader.setCurve(CrossfadeCurve::EqualPower);
  crossfader.beginCrossfade(100);
  EXPECT_TRUE(crossfader.isCrossfadeActive());

  std::vector<float> incoming(100 * 2, 0.8f);
  crossfader.writeIncomingSamples(incoming.data(), incoming.size());

  std::vector<float> outgoing(50 * 2, 0.5f);
  crossfader.process(outgoing.data(), 50, 2);

  // Outgoing buffer should be smoothly blended with incoming
  EXPECT_NE(outgoing[25 * 2], 0.5f);
  EXPECT_TRUE(crossfader.isCrossfadeActive());

  crossfader.reset();
  EXPECT_FALSE(crossfader.isCrossfadeActive());
}

TEST(AudioPipelineTest, CueSheetParserIndexAndVirtualTracks) {
  std::string cue = R"(
PERFORMER "Pink Floyd"
TITLE "Dark Side of the Moon"
FILE "album.flac" WAVE
  TRACK 01 AUDIO
    TITLE "Speak to Me"
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    TITLE "Breathe"
    PERFORMER "Pink Floyd"
    SONGWRITER "Waters/Gilmour/Wright"
    INDEX 01 01:13:00
  TRACK 03 AUDIO
    TITLE "On the Run"
    INDEX 01 03:56:50
)";

  uint64_t ms1 = CueSheetParser::parseIndexTimestampMs("01:13:00");
  EXPECT_EQ(ms1, (1ULL * 60 + 13) * 1000); // 73,000 ms

  uint64_t ms2 = CueSheetParser::parseIndexTimestampMs("03:56:50");
  EXPECT_EQ(ms2, (3ULL * 60 + 56) * 1000 + (50 * 1000) / 75);

  auto tracks = CueSheetParser::parseString(cue, "/music/album.flac", 300000);
  ASSERT_EQ(tracks.size(), 3u);

  EXPECT_EQ(tracks[0].title, "Speak to Me");
  EXPECT_EQ(tracks[0].album, "Dark Side of the Moon");
  EXPECT_EQ(tracks[0].artist, "Pink Floyd");
  EXPECT_EQ(tracks[0].startTimeMs, 0u);
  EXPECT_EQ(tracks[0].endTimeMs, 73000u);
  EXPECT_EQ(tracks[0].durationMs, 73000u);
  EXPECT_TRUE(tracks[0].isVirtualTrack);

  EXPECT_EQ(tracks[1].title, "Breathe");
  EXPECT_EQ(tracks[1].startTimeMs, 73000u);
  EXPECT_GT(tracks[1].durationMs, 0u);

  EXPECT_EQ(tracks[2].title, "On the Run");
  EXPECT_EQ(tracks[2].endTimeMs, 300000u);
}

TEST(AudioPipelineTest, WaveformExtractorPeakCalculationAndCache) {
  std::filesystem::path tempCacheDir =
      std::filesystem::temp_directory_path() / "waveform_cache_test";
  WaveformExtractor extractor(tempCacheDir.string());
  EXPECT_EQ(extractor.getCacheDirectory(), tempCacheDir.string());

  std::vector<float> peaks =
      extractor.extractPeaks("/music/non_existent.flac", 128);
  EXPECT_EQ(peaks.size(), 128u);
  for (float p : peaks) {
    EXPECT_GE(p, 0.0f);
    EXPECT_LE(p, 1.0f);
  }

  // Second extraction loads from disk cache
  std::vector<float> cachedPeaks =
      extractor.extractPeaks("/music/non_existent.flac", 128);
  EXPECT_EQ(peaks, cachedPeaks);

  std::filesystem::remove_all(tempCacheDir);
}

TEST(AudioPipelineTest, DirectHardwareAudioEngineFormalNamingAndBufferHealth) {
  ensureTestCoreApp();
  auto engine = std::make_shared<DirectHardwareAudioEngine>();
  EXPECT_TRUE(engine->initialize(48000, 2));

  // Test formal aliases
  std::shared_ptr<StudioMasterAudioEngine> studioEngine = engine;
  EXPECT_NE(studioEngine, nullptr);

  // Diagnostics & Buffer Health
  EXPECT_FLOAT_EQ(engine->getBufferFillPercentage(), 0.0f);
  EXPECT_FALSE(engine->getActiveDecoderName().empty());

  // Crossfade and ReplayGain core controls
  engine->setCrossfadeEnabled(true);
  EXPECT_TRUE(engine->isCrossfadeEnabled());
  engine->setCrossfadeDurationSec(3.5f);
  EXPECT_FLOAT_EQ(engine->getCrossfadeDurationSec(), 3.5f);

  engine->setReplayGainMode(2); // Album Gain
  EXPECT_EQ(engine->getReplayGainMode(), 2);
}
