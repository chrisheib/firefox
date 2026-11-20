/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AudioTimeStretcher.h"

#include "RLBoxSoundTouch.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPrefs_media.h"
#include "nsString.h"

#include <bungee/Bungee.h>
#include <bungee/Stream.h>
#include <signalsmith-stretch/signalsmith-stretch.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#define PREF_AUDIO_ENGINE "media.audio_engine"
#define PREF_SIGNALSMITH_BLOCK_MS \
  "media.audio.playbackrate.signalsmith_block_ms"
#define PREF_SIGNALSMITH_INTERVAL_MS \
  "media.audio.playbackrate.signalsmith_interval_ms"
#define PREF_SIGNALSMITH_SPLIT_COMPUTATION \
  "media.audio.playbackrate.signalsmith_split_computation"
#define PREF_BUNGEE_HOP_ADJUST \
  "media.audio.playbackrate.bungee_hop_adjust"
#define PREF_BUNGEE_CHUNK_FRAMES \
  "media.audio.playbackrate.bungee_chunk_frames"

namespace mozilla {

namespace {

constexpr const char* FF_UPSTREAM_ENGINE = "ff-upstream";
constexpr const char* SIGNALSMITH_ENGINE = "signalsmith-stretch";
constexpr const char* BUNGEE_ENGINE = "bungee";

class TimeStretcherImpl {
 public:
  virtual ~TimeStretcherImpl() = default;
  virtual bool Init(uint32_t aSampleRate, uint32_t aChannels) = 0;
  virtual void SetTempo(double aTempo) = 0;
  virtual void SetRate(double aRate) = 0;
  virtual uint32_t NumSamples() const = 0;
  virtual uint32_t NumUnprocessedSamples() const = 0;
  virtual void PutSamples(const AudioDataValue* aSamples,
                          uint32_t aNumSamples) = 0;
  virtual uint32_t ReceiveSamples(AudioDataValue* aOutput,
                                  uint32_t aMaxSamples) = 0;
  virtual void Clear() = 0;
  virtual void Flush() = 0;
  virtual uint32_t InitialDelayFrames() const = 0;
};

class SoundTouchTimeStretcher final : public TimeStretcherImpl {
 public:
  bool Init(uint32_t aSampleRate, uint32_t aChannels) override {
    mSampleRate = aSampleRate;
    mChannels = aChannels;
    mSoundTouch = std::make_unique<RLBoxSoundTouch>();
    if (!mSoundTouch->Init()) {
      return false;
    }
    mSoundTouch->setSampleRate(aSampleRate);
    mSoundTouch->setChannels(aChannels);
    mSoundTouch->setPitch(1.0);
    mSoundTouch->setSetting(
        SETTING_SEQUENCE_MS,
        StaticPrefs::media_audio_playbackrate_soundtouch_sequence_ms());
    mSoundTouch->setSetting(
        SETTING_SEEKWINDOW_MS,
        StaticPrefs::media_audio_playbackrate_soundtouch_seekwindow_ms());
    mSoundTouch->setSetting(
        SETTING_OVERLAP_MS,
        StaticPrefs::media_audio_playbackrate_soundtouch_overlap_ms());
    return true;
  }

  void SetTempo(double aTempo) override {
    mSoundTouch->setTempo(aTempo);
  }

  void SetRate(double aRate) override { mSoundTouch->setRate(aRate); }

  uint32_t NumSamples() const override {
    return mSoundTouch->numSamples().unverified_safe_because(
        "Checked by the media graph consumer before use.");
  }

  uint32_t NumUnprocessedSamples() const override {
    return mSoundTouch->numUnprocessedSamples().unverified_safe_because(
        "Checked when deciding whether to flush.");
  }

  void PutSamples(const AudioDataValue* aSamples,
                  uint32_t aNumSamples) override {
    mSoundTouch->putSamples(aSamples, aNumSamples);
  }

  uint32_t ReceiveSamples(AudioDataValue* aOutput,
                          uint32_t aMaxSamples) override {
    return mSoundTouch->receiveSamples(aOutput, aMaxSamples);
  }

  void Clear() override { mSoundTouch->clear(); }
  void Flush() override { mSoundTouch->flush(); }

  uint32_t InitialDelayFrames() const override {
    uint64_t milliseconds =
        uint64_t(StaticPrefs::
                     media_audio_playbackrate_soundtouch_sequence_ms()) +
        StaticPrefs::media_audio_playbackrate_soundtouch_seekwindow_ms();
    uint64_t frames = milliseconds * mSampleRate / 1000;
    return uint32_t(std::min<uint64_t>(frames, UINT32_MAX));
  }

 private:
  std::unique_ptr<RLBoxSoundTouch> mSoundTouch;
  uint32_t mSampleRate = 0;
  uint32_t mChannels = 0;
};

class BufferedTimeStretcher : public TimeStretcherImpl {
 public:
  uint32_t NumSamples() const override {
    return uint32_t((mOutput.size() - mOutputOffset) / mChannels);
  }

  uint32_t NumUnprocessedSamples() const override {
    return mNeedsFlush ? 1 : 0;
  }

  uint32_t ReceiveSamples(AudioDataValue* aOutput,
                          uint32_t aMaxSamples) override {
    uint32_t frames = std::min(aMaxSamples, NumSamples());
    size_t samples = size_t(frames) * mChannels;
    if (samples) {
      std::copy_n(mOutput.begin() + mOutputOffset, samples, aOutput);
      mOutputOffset += samples;
      if (mOutputOffset == mOutput.size()) {
        mOutput.clear();
        mOutputOffset = 0;
      } else if (mOutputOffset > 1 << 20) {
        mOutput.erase(mOutput.begin(),
                      mOutput.begin() + std::ptrdiff_t(mOutputOffset));
        mOutputOffset = 0;
      }
    }
    return frames;
  }

  void SetTempo(double aTempo) override {
    mTempo = std::max(aTempo, 0.01);
  }

  void SetRate(double aRate) override {
    mRate = std::max(aRate, 0.01);
  }

  void Clear() override {
    mOutput.clear();
    mOutputOffset = 0;
    mOutputFraction = 0;
    mNeedsFlush = false;
    ResetBackend();
  }

 protected:
  uint32_t OutputFramesForInput(uint32_t aInputFrames) {
    mOutputFraction += double(aInputFrames) / Speed();
    uint32_t outputFrames = uint32_t(std::floor(mOutputFraction));
    mOutputFraction -= outputFrames;
    return outputFrames;
  }

  void AppendOutput(const std::vector<float>& aPlanar,
                    uint32_t aOutputFrames) {
    if (!aOutputFrames) {
      return;
    }
    mOutput.reserve(mOutput.size() + size_t(aOutputFrames) * mChannels);
    for (uint32_t frame = 0; frame < aOutputFrames; ++frame) {
      for (uint32_t channel = 0; channel < mChannels; ++channel) {
        mOutput.push_back(aPlanar[size_t(channel) * aOutputFrames + frame]);
      }
    }
  }

  double Speed() const { return mTempo * mRate; }
  double mTempo = 1;
  double mRate = 1;
  uint32_t mSampleRate = 0;
  uint32_t mChannels = 0;
  std::vector<AudioDataValue> mOutput;
  size_t mOutputOffset = 0;
  double mOutputFraction = 0;
  bool mNeedsFlush = false;

 private:
  virtual void ResetBackend() = 0;
};

class SignalsmithTimeStretcher final : public BufferedTimeStretcher {
 public:
  bool Init(uint32_t aSampleRate, uint32_t aChannels) override {
    mSampleRate = aSampleRate;
    mChannels = aChannels;
    int blockSamples =
        std::max(32, int(aSampleRate *
                         Preferences::GetInt(PREF_SIGNALSMITH_BLOCK_MS, 120) /
                         1000));
    int intervalSamples =
        std::max(
            1, int(aSampleRate *
                   Preferences::GetInt(PREF_SIGNALSMITH_INTERVAL_MS, 30) /
                   1000));
    intervalSamples = std::min(intervalSamples, blockSamples / 2);
    mStretcher.configure(
        int(aChannels), blockSamples, intervalSamples,
        Preferences::GetBool(PREF_SIGNALSMITH_SPLIT_COMPUTATION, false));
    return true;
  }

  void PutSamples(const AudioDataValue* aSamples,
                  uint32_t aNumSamples) override {
    if (!aNumSamples) {
      return;
    }
    uint32_t outputFrames = OutputFramesForInput(aNumSamples);
    Process(aSamples, aNumSamples, outputFrames);
    mNeedsFlush = true;
  }

  void Flush() override {
    if (!mNeedsFlush) {
      return;
    }
    std::vector<AudioDataValue> silence(
        size_t(mStretcher.inputLatency()) * mChannels);
    Process(silence.data(), uint32_t(mStretcher.inputLatency()),
            OutputFramesForInput(uint32_t(mStretcher.inputLatency())));

    std::vector<float> outputPlanar(
        size_t(mChannels) * mStretcher.outputLatency());
    std::vector<float*> outputPointers(mChannels);
    for (uint32_t channel = 0; channel < mChannels; ++channel) {
      outputPointers[channel] =
          outputPlanar.data() + size_t(channel) * mStretcher.outputLatency();
    }
    mStretcher.flush(outputPointers.data(), int(mStretcher.outputLatency()));
    AppendOutput(outputPlanar, uint32_t(mStretcher.outputLatency()));
    mNeedsFlush = false;
  }

  uint32_t InitialDelayFrames() const override {
    return uint32_t(mStretcher.inputLatency() + mStretcher.outputLatency());
  }

 private:
  void Process(const AudioDataValue* aSamples, uint32_t aInputFrames,
               uint32_t aOutputFrames) {
    std::vector<float> inputPlanar(size_t(aInputFrames) * mChannels);
    std::vector<float> outputPlanar(size_t(aOutputFrames) * mChannels);
    std::vector<const float*> inputPointers(mChannels);
    std::vector<float*> outputPointers(mChannels);
    for (uint32_t channel = 0; channel < mChannels; ++channel) {
      inputPointers[channel] =
          inputPlanar.data() + size_t(channel) * aInputFrames;
      outputPointers[channel] =
          outputPlanar.data() + size_t(channel) * aOutputFrames;
      for (uint32_t frame = 0; frame < aInputFrames; ++frame) {
        inputPlanar[size_t(channel) * aInputFrames + frame] =
            aSamples[size_t(frame) * mChannels + channel];
      }
    }
    mStretcher.setTransposeFactor(float(mRate));
    mStretcher.process(inputPointers.data(), int(aInputFrames),
                       outputPointers.data(), int(aOutputFrames));
    AppendOutput(outputPlanar, aOutputFrames);
  }

  void ResetBackend() override { mStretcher.reset(); }

  signalsmith::stretch::SignalsmithStretch<float> mStretcher;
};

class BungeeTimeStretcher final : public BufferedTimeStretcher {
 public:
  bool Init(uint32_t aSampleRate, uint32_t aChannels) override {
    mSampleRate = aSampleRate;
    mChannels = aChannels;
    mChunkFrames =
        std::max(128, Preferences::GetInt(PREF_BUNGEE_CHUNK_FRAMES, 2048));
    int hopAdjust =
        std::clamp(Preferences::GetInt(PREF_BUNGEE_HOP_ADJUST, 0), -1, 1);
    mStretcher = std::make_unique<Bungee::Stretcher<Bungee::Basic>>(
        Bungee::SampleRates{int(aSampleRate), int(aSampleRate)}, int(aChannels),
        hopAdjust);
    mStream = std::make_unique<Bungee::Stream<Bungee::Basic>>(
        *mStretcher, mChunkFrames, int(aChannels));
    return true;
  }

  void PutSamples(const AudioDataValue* aSamples,
                  uint32_t aNumSamples) override {
    for (uint32_t offset = 0; offset < aNumSamples;) {
      uint32_t frames = std::min(mChunkFrames, aNumSamples - offset);
      Process(aSamples + size_t(offset) * mChannels, frames,
              OutputFramesForInput(frames));
      offset += frames;
    }
    mNeedsFlush = mNeedsFlush || aNumSamples;
  }

  void Flush() override {
    if (!mNeedsFlush) {
      return;
    }
    std::vector<AudioDataValue> silence(size_t(mChunkFrames) * mChannels);
    for (uint32_t pass = 0; pass < 2; ++pass) {
      Process(silence.data(), mChunkFrames,
              std::max(1u, OutputFramesForInput(mChunkFrames)));
    }
    mNeedsFlush = false;
  }

  uint32_t InitialDelayFrames() const override {
    return uint32_t(std::max(1, mStretcher->maxInputFrameCount()));
  }

 private:
  void Process(const AudioDataValue* aSamples, uint32_t aInputFrames,
               uint32_t aOutputFrames) {
    aOutputFrames = std::max(aOutputFrames, 1u);
    std::vector<float> inputPlanar(size_t(aInputFrames) * mChannels);
    std::vector<float> outputPlanar(size_t(aOutputFrames) * mChannels);
    std::vector<const float*> inputPointers(mChannels);
    std::vector<float*> outputPointers(mChannels);
    for (uint32_t channel = 0; channel < mChannels; ++channel) {
      inputPointers[channel] =
          inputPlanar.data() + size_t(channel) * aInputFrames;
      outputPointers[channel] =
          outputPlanar.data() + size_t(channel) * aOutputFrames;
      for (uint32_t frame = 0; frame < aInputFrames; ++frame) {
        inputPlanar[size_t(channel) * aInputFrames + frame] =
            aSamples[size_t(frame) * mChannels + channel];
      }
    }
    uint32_t frames = uint32_t(mStream->process(
        inputPointers.data(), outputPointers.data(), int(aInputFrames),
        double(aOutputFrames), mRate));
    AppendOutput(outputPlanar, frames);
  }

  void ResetBackend() override {
    int sampleRate = int(mSampleRate);
    int channels = int(mChannels);
    int hopAdjust =
        std::clamp(Preferences::GetInt(PREF_BUNGEE_HOP_ADJUST, 0), -1, 1);
    mStretcher = std::make_unique<Bungee::Stretcher<Bungee::Basic>>(
        Bungee::SampleRates{sampleRate, sampleRate}, channels, hopAdjust);
    mStream = std::make_unique<Bungee::Stream<Bungee::Basic>>(
        *mStretcher, mChunkFrames, channels);
  }

  uint32_t mChunkFrames = 2048;
  std::unique_ptr<Bungee::Stretcher<Bungee::Basic>> mStretcher;
  std::unique_ptr<Bungee::Stream<Bungee::Basic>> mStream;
};

}  // namespace

class AudioTimeStretcher::Impl {
 public:
  explicit Impl(std::unique_ptr<TimeStretcherImpl> aBackend)
      : mBackend(std::move(aBackend)) {}

  bool Init(uint32_t aSampleRate, uint32_t aChannels) {
    return mBackend->Init(aSampleRate, aChannels);
  }
  void SetTempo(double aTempo) { mBackend->SetTempo(aTempo); }
  void SetRate(double aRate) { mBackend->SetRate(aRate); }
  uint32_t NumSamples() const { return mBackend->NumSamples(); }
  uint32_t NumUnprocessedSamples() const {
    return mBackend->NumUnprocessedSamples();
  }
  void PutSamples(const AudioDataValue* aSamples, uint32_t aNumSamples) {
    mBackend->PutSamples(aSamples, aNumSamples);
  }
  uint32_t ReceiveSamples(AudioDataValue* aOutput, uint32_t aMaxSamples) {
    return mBackend->ReceiveSamples(aOutput, aMaxSamples);
  }
  void Clear() { mBackend->Clear(); }
  void Flush() { mBackend->Flush(); }
  uint32_t InitialDelayFrames() const {
    return mBackend->InitialDelayFrames();
  }

 private:
  std::unique_ptr<TimeStretcherImpl> mBackend;
};

AudioTimeStretcher::AudioTimeStretcher() = default;
AudioTimeStretcher::~AudioTimeStretcher() = default;

bool AudioTimeStretcher::Init(uint32_t aSampleRate, uint32_t aChannels) {
  nsAutoCString engine;
  Preferences::GetCString(PREF_AUDIO_ENGINE, engine);
  if (engine.Equals(FF_UPSTREAM_ENGINE)) {
    mImpl = MakeUnique<Impl>(std::make_unique<SoundTouchTimeStretcher>());
  } else if (engine.Equals(SIGNALSMITH_ENGINE)) {
    mImpl = MakeUnique<Impl>(std::make_unique<SignalsmithTimeStretcher>());
  } else if (engine.Equals(BUNGEE_ENGINE)) {
    mImpl = MakeUnique<Impl>(std::make_unique<BungeeTimeStretcher>());
  } else {
    mImpl = MakeUnique<Impl>(std::make_unique<SoundTouchTimeStretcher>());
  }
  if (!mImpl->Init(aSampleRate, aChannels)) {
    mImpl = nullptr;
    return false;
  }
  return true;
}

void AudioTimeStretcher::setTempo(double aTempo) { mImpl->SetTempo(aTempo); }
void AudioTimeStretcher::setRate(double aRate) { mImpl->SetRate(aRate); }
uint32_t AudioTimeStretcher::numSamples() const {
  return mImpl ? mImpl->NumSamples() : 0;
}
uint32_t AudioTimeStretcher::numUnprocessedSamples() const {
  return mImpl ? mImpl->NumUnprocessedSamples() : 0;
}
void AudioTimeStretcher::putSamples(const AudioDataValue* aSamples,
                                    uint32_t aNumSamples) {
  mImpl->PutSamples(aSamples, aNumSamples);
}
uint32_t AudioTimeStretcher::receiveSamples(AudioDataValue* aOutput,
                                            uint32_t aMaxSamples) {
  return mImpl->ReceiveSamples(aOutput, aMaxSamples);
}
void AudioTimeStretcher::clear() { mImpl->Clear(); }
void AudioTimeStretcher::flush() { mImpl->Flush(); }
uint32_t AudioTimeStretcher::initialDelayFrames() const {
  return mImpl ? mImpl->InitialDelayFrames() : 0;
}

}  // namespace mozilla
