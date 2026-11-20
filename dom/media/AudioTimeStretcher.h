/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AudioTimeStretcher_h_
#define AudioTimeStretcher_h_

#include "AudioSampleFormat.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {

class AudioTimeStretcher {
 public:
  AudioTimeStretcher();
  ~AudioTimeStretcher();

  bool Init(uint32_t aSampleRate, uint32_t aChannels);
  void setTempo(double aTempo);
  void setRate(double aRate);
  uint32_t numSamples() const;
  uint32_t numUnprocessedSamples() const;
  void putSamples(const AudioDataValue* aSamples, uint32_t aNumSamples);
  uint32_t receiveSamples(AudioDataValue* aOutput, uint32_t aMaxSamples);
  void clear();
  void flush();
  uint32_t initialDelayFrames() const;

 private:
  class Impl;
  UniquePtr<Impl> mImpl;
};

}  // namespace mozilla

#endif
