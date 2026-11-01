#include <chrono>
#include <gtest/gtest.h>
#include <numeric>
#include <thread>
#include <vector>

#include "audio/AudioThreadPriority.hpp"
#include "audio/BitPerfectAudioAdapter.hpp"
#include "audio/LockFreeRingBuffer.hpp"
#include "audio/alsa/AlsaAudioSink.hpp"
#include "audio/dsp/ChannelMatrixProcessor.hpp"
#include "audio/pipewire/PipeWireAudioSink.hpp"
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
