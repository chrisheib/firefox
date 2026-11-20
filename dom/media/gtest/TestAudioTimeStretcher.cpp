/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AudioTimeStretcher.h"
#include "gtest/gtest.h"
#include "mozilla/Preferences.h"

#include <cmath>
#include <vector>

using namespace mozilla;

TEST(AudioTimeStretcher, ProvidersProduceOutput)
{
  static constexpr const char* engines[] = {
      "ff-upstream",
      "signalsmith-stretch",
      "bungee",
  };
  static constexpr uint32_t kSampleRate = 48000;
  static constexpr uint32_t kChannels = 2;
  static constexpr uint32_t kChunkFrames = 1024;
  static constexpr uint32_t kChunks = 32;

  for (const char* engine : engines) {
    Preferences::SetCString("media.audio_engine", engine);
    AudioTimeStretcher stretcher;
    ASSERT_TRUE(stretcher.Init(kSampleRate, kChannels));
    stretcher.setTempo(1.5);
    stretcher.setRate(1.0);

    std::vector<float> input(kChunkFrames * kChannels);
    std::vector<float> output((kChunkFrames + 1024) * kChannels);
    uint32_t received = 0;
    for (uint32_t chunk = 0; chunk < kChunks; ++chunk) {
      for (uint32_t frame = 0; frame < kChunkFrames; ++frame) {
        float phase =
            float(2 * M_PI * 440 * (chunk * kChunkFrames + frame)) /
            kSampleRate;
        input[frame * kChannels] = std::sin(phase) * 0.25f;
        input[frame * kChannels + 1] = std::sin(phase * 0.5f) * 0.25f;
      }
      stretcher.putSamples(input.data(), kChunkFrames);
      uint32_t available = stretcher.numSamples();
      if (available) {
        uint32_t frames =
            stretcher.receiveSamples(output.data(), output.size() / kChannels);
        received += frames;
      }
    }

    stretcher.flush();
    received += stretcher.receiveSamples(output.data(), output.size() / kChannels);
    EXPECT_GT(received, kChunkFrames);
  }

  Preferences::ClearUser("media.audio_engine");
}
