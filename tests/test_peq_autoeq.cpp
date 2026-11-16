#include <QCoreApplication>
#include <cmath>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "audio/dsp/AutoEqDatabase.hpp"
#include "audio/dsp/EqFileParser.hpp"
#include "dsp/equalizerdsp.h"

using namespace adapters;

static void ensureTestApp() {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (!QCoreApplication::instance()) {
    static int argc = 1;
    static char appName[] = "picaudio_tests";
    static char *argv[] = {appName, nullptr};
    new QCoreApplication(argc, argv);
  }
}

TEST(PeqAutoEqTest, ParseEqualizerApoText) {
  const std::string apoText =
      "Preamp: -6.5 dB\n"
      "Filter 1: ON PK Fc 28 Hz Gain 4.5 dB Q 1.41\n"
      "Filter 2: ON LSC Fc 105 Hz Gain 2.0 dB Q 0.71\n"
      "Filter 3: ON HSC Fc 8000 Hz Gain -3.0 dB Q 0.71\n"
      "Filter 4: ON PK Fc 3000 Hz Gain -2.5 dB Q 2.00\n";

  auto result = EqFileParser::parse(apoText);
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.mode, 1); // Parametric
  EXPECT_NEAR(result.preampDb, -6.5f, 0.01f);
  ASSERT_EQ(result.peqBands.size(), 4u);

  // Band 1: Peaking 28Hz, +4.5dB, Q 1.41
  EXPECT_EQ(result.peqBands[0].type, core::EqFilterType::Peaking);
  EXPECT_NEAR(result.peqBands[0].frequency, 28.0f, 0.1f);
  EXPECT_NEAR(result.peqBands[0].gainDb, 4.5f, 0.01f);
  EXPECT_NEAR(result.peqBands[0].q, 1.41f, 0.01f);
  EXPECT_TRUE(result.peqBands[0].enabled);

  // Band 2: Low Shelf 105Hz, +2.0dB, Q 0.71
  EXPECT_EQ(result.peqBands[1].type, core::EqFilterType::LowShelf);
  EXPECT_NEAR(result.peqBands[1].frequency, 105.0f, 0.1f);
  EXPECT_NEAR(result.peqBands[1].gainDb, 2.0f, 0.01f);
  EXPECT_NEAR(result.peqBands[1].q, 0.71f, 0.01f);

  // Band 3: High Shelf 8000Hz, -3.0dB, Q 0.71
  EXPECT_EQ(result.peqBands[2].type, core::EqFilterType::HighShelf);
  EXPECT_NEAR(result.peqBands[2].frequency, 8000.0f, 0.1f);
  EXPECT_NEAR(result.peqBands[2].gainDb, -3.0f, 0.01f);

  // Band 4: Peaking 3000Hz, -2.5dB, Q 2.00
  EXPECT_EQ(result.peqBands[3].type, core::EqFilterType::Peaking);
  EXPECT_NEAR(result.peqBands[3].frequency, 3000.0f, 0.1f);
  EXPECT_NEAR(result.peqBands[3].gainDb, -2.5f, 0.01f);
  EXPECT_NEAR(result.peqBands[3].q, 2.0f, 0.01f);
}

TEST(PeqAutoEqTest, ParseSquiglinkGraphicEqText) {
  const std::string graphicEqText =
      "GraphicEQ: 20 0; 50 2.5; 100 3.0; 250 1.0; 500 0; 1000 -1.5; 2000 2.0; "
      "4000 4.0; 8000 -2.0; 16000 -5.0";

  auto result = EqFileParser::parse(graphicEqText);
  EXPECT_TRUE(result.success);
  // Peak positive gain was +4.0 dB, so preamp should be -4.2 dB (+0.2 dB safety
  // headroom)
  EXPECT_NEAR(result.preampDb, -4.2f, 0.01f);
  EXPECT_FALSE(result.graphicGains.empty());
}

TEST(PeqAutoEqTest, ExportEqualizerApoText) {
  std::vector<core::PeqFilterBand> bands = {
      {core::EqFilterType::Peaking, 1000.0f, 3.5f, 1.41f, true},
      {core::EqFilterType::LowShelf, 105.0f, 2.0f, 0.71f, true},
      {core::EqFilterType::HighShelf, 10000.0f, -1.5f, 0.71f, false}};

  std::string exported = EqFileParser::exportEqualizerApo(bands, -3.5f);
  EXPECT_NE(exported.find("Preamp: -3.5 dB"), std::string::npos);
  EXPECT_NE(exported.find("Filter 1: ON PK Fc 1000 Hz Gain 3.5 dB Q 1.41"),
            std::string::npos);
  EXPECT_NE(exported.find("Filter 2: ON LSC Fc 105 Hz Gain 2.0 dB Q 0.71"),
            std::string::npos);
  EXPECT_NE(exported.find("Filter 3: OFF HSC Fc 10000 Hz Gain -1.5 dB Q 0.71"),
            std::string::npos);
}

TEST(PeqAutoEqTest, EqualizerDspBiquadCalculation) {
  PicAudioEngine::EqualizerDsp eq;
  eq.setEqMode(1); // Parametric

  std::vector<PicAudioEngine::PeqFilterSpec> filters = {
      {PicAudioEngine::PeqFilterType::Peaking, 1000.0f, 6.0f, 1.0f, true}};
  eq.setPeqFilters(filters);

  EXPECT_EQ(eq.eqMode(), 1);
  EXPECT_EQ(eq.peqFilters().size(), 1u);

  // Calculate frequency response at 1 kHz -> should be close to +6 dB
  std::vector<float> freqs = {100.0f, 1000.0f, 10000.0f};
  auto resp = PicAudioEngine::EqualizerDsp::calculateResponse(filters, 0.0f,
                                                              freqs, 48000);
  ASSERT_EQ(resp.size(), 3u);

  // At 1000 Hz, response should be around +6 dB
  EXPECT_NEAR(resp[1], 6.0f, 0.25f);
  // Away from 1000 Hz, response should fall towards 0 dB
  EXPECT_LT(std::abs(resp[0]), 2.0f);
  EXPECT_LT(std::abs(resp[2]), 2.0f);
}

TEST(PeqAutoEqTest, EqualizerDspSettingsSerialization) {
  PicAudioEngine::EqualizerDsp eq;
  eq.setEqMode(1); // Parametric

  std::vector<PicAudioEngine::PeqFilterSpec> filters = {
      {PicAudioEngine::PeqFilterType::LowShelf, 105.0f, 4.0f, 0.71f, true},
      {PicAudioEngine::PeqFilterType::Peaking, 2500.0f, -3.0f, 2.0f, true},
      {PicAudioEngine::PeqFilterType::HighShelf, 8000.0f, 1.5f, 0.71f, false}};
  eq.setPeqFilters(filters);

  QByteArray savedJson = eq.saveSettings();
  EXPECT_TRUE(savedJson.contains("\"mode\":1"));
  EXPECT_TRUE(savedJson.contains("\"filters\""));

  // Create another instance and load settings
  PicAudioEngine::EqualizerDsp eq2;
  EXPECT_TRUE(eq2.loadSettings(savedJson));

  EXPECT_EQ(eq2.eqMode(), 1);
  auto loadedFilters = eq2.peqFilters();
  ASSERT_EQ(loadedFilters.size(), 3u);
  EXPECT_EQ(loadedFilters[0].type, PicAudioEngine::PeqFilterType::LowShelf);
  EXPECT_NEAR(loadedFilters[0].frequency, 105.0f, 0.1f);
  EXPECT_NEAR(loadedFilters[0].gainDb, 4.0f, 0.01f);
  EXPECT_NEAR(loadedFilters[0].q, 0.71f, 0.01f);
  EXPECT_TRUE(loadedFilters[0].enabled);

  EXPECT_EQ(loadedFilters[2].type, PicAudioEngine::PeqFilterType::HighShelf);
  EXPECT_FALSE(loadedFilters[2].enabled);
}

TEST(PeqAutoEqTest, AutoEqDatabaseBundledProfiles) {
  ensureTestApp();

  auto &db = AutoEqDatabase::instance();
  db.initialize();
  EXPECT_GE(db.profileCount(), 20u);

  // Search for Sennheiser HD600
  auto hdResults = db.search("HD600", 5);
  ASSERT_FALSE(hdResults.empty());
  bool foundHd600 = false;
  for (const auto &r : hdResults) {
    if (r.model.contains("HD 600") || r.model.contains("HD600")) {
      foundHd600 = true;
      EXPECT_FALSE(r.filters.empty());
      EXPECT_LE(r.preampDb, 0.0f);
      break;
    }
  }
  EXPECT_TRUE(foundHd600);

  // Search for Moondrop Blessing 2
  auto b2Results = db.search("Blessing 2", 5);
  ASSERT_FALSE(b2Results.empty());
  bool foundB2 = false;
  for (const auto &r : b2Results) {
    if (r.model.contains("Blessing 2")) {
      foundB2 = true;
      EXPECT_FALSE(r.filters.empty());
      break;
    }
  }
  EXPECT_TRUE(foundB2);

  // Search for Tangzu Wan'er
  auto wanerResults = db.search("Waner", 5);
  ASSERT_FALSE(wanerResults.empty());

  // Empty search should return top profiles for immediate UI display
  auto emptyResults = db.search("", 30);
  EXPECT_GE(emptyResults.size(), 20u);
  for (const auto &r : emptyResults) {
    EXPECT_FALSE(r.id.isEmpty());
    EXPECT_FALSE(r.model.isEmpty());
    EXPECT_FALSE(r.source.isEmpty());
  }
}

TEST(PeqAutoEqTest, AutoEqLowShelfStabilityAndProcessing) {
  PicAudioEngine::EqualizerDsp eq;
  eq.setEqMode(1); // Parametric mode

  // Standard AutoEq profile filters (e.g., IEM Harman target bass boost)
  std::vector<PicAudioEngine::PeqFilterSpec> filters = {
      {PicAudioEngine::PeqFilterType::LowShelf, 105.0f, 6.5f, 0.71f, true},
      {PicAudioEngine::PeqFilterType::Peaking, 1000.0f, -2.0f, 1.41f, true},
      {PicAudioEngine::PeqFilterType::HighShelf, 8000.0f, -3.0f, 0.71f, true}};
  eq.setPeqFilters(filters);
  eq.setPreampGain(-6.5f);

  // Frequency response verification
  std::vector<float> testFreqs = {40.0f, 1000.0f, 8000.0f};
  auto resp = PicAudioEngine::EqualizerDsp::calculateResponse(filters, -6.5f,
                                                              testFreqs, 44100);
  ASSERT_EQ(resp.size(), 3u);
  // Bass at 40 Hz should have +6.5 dB shelf offset by -6.5 dB preamp ~ 0 dB net
  EXPECT_NEAR(resp[0], 0.0f, 0.5f);
  // 1 kHz with -2 dB peaking - 6.5 dB preamp ~ -8.5 dB
  EXPECT_NEAR(resp[1], -8.5f, 0.5f);
  // 8 kHz is the transition midpoint of the -3 dB shelf (approx -1.5 dB) - 6.5
  // dB preamp ~ -8.0 dB
  EXPECT_NEAR(resp[2], -8.0f, 0.5f);

  // Now test live audio processing stability over 20,000 frames (approx 0.5
  // seconds at 44.1 kHz)
  PicAudioEngine::AudioFormat format{PicAudioEngine::SampleFormat::F32, 44100,
                                     2};
  eq.prepare(format);

  const int totalFrames = 22050; // 0.5s of audio
  std::vector<double> inputSamples(totalFrames * 2);
  for (int i = 0; i < totalFrames; ++i) {
    // 100 Hz test tone at 0.5 amplitude
    double val = 0.5 * std::sin(2.0 * std::numbers::pi * 100.0 * (i / 44100.0));
    inputSamples[static_cast<size_t>(i * 2)] = val;
    inputSamples[static_cast<size_t>(i * 2 + 1)] = val;
  }

  PicAudioEngine::ProcessingBuffer buffer(inputSamples, format, 0);
  PicAudioEngine::ProcessingBufferList chunks;
  chunks.addChunk(std::move(buffer));

  // Process the audio through the DSP
  eq.process(chunks);

  ASSERT_EQ(chunks.count(), 1u);
  auto *outBuf = chunks.item(0);
  ASSERT_NE(outBuf, nullptr);
  auto outSpan = outBuf->data();
  ASSERT_EQ(outSpan.size(), static_cast<size_t>(totalFrames * 2));

  // Verify EVERY single sample is strictly finite, non-NaN, and stable
  bool allFinite = true;
  double maxAbs = 0.0;
  for (double s : outSpan) {
    if (!std::isfinite(s)) {
      allFinite = false;
      break;
    }
    maxAbs = std::max(maxAbs, std::abs(s));
  }

  EXPECT_TRUE(allFinite);
  // Audio must remain within reasonable bounds (neither silent/zeroed out nor
  // exploded)
  EXPECT_GT(maxAbs, 0.01);
  EXPECT_LE(maxAbs, 1.25);

  // Recovery test: verify that a poisoned NaN sample does not cause permanent
  // audio death
  std::vector<double> poisonSamples = {
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::quiet_NaN()};
  PicAudioEngine::ProcessingBuffer poisonBuf(poisonSamples, format, 0);
  PicAudioEngine::ProcessingBufferList poisonChunks;
  poisonChunks.addChunk(std::move(poisonBuf));
  eq.process(poisonChunks);

  // Subsequent valid buffer must immediately be clean and finite
  std::vector<double> validSamples = {0.3, 0.3, 0.2, 0.2};
  PicAudioEngine::ProcessingBuffer validBuf(validSamples, format, 0);
  PicAudioEngine::ProcessingBufferList validChunks;
  validChunks.addChunk(std::move(validBuf));
  eq.process(validChunks);

  auto *recoveredBuf = validChunks.item(0);
  ASSERT_NE(recoveredBuf, nullptr);
  for (double s : recoveredBuf->data()) {
    EXPECT_TRUE(std::isfinite(s));
  }
}
