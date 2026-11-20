/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const {
  AudioEngine,
  AUDIO_ENGINE_PREF,
  BUNGEE_ENGINE_ID,
  FF_UPSTREAM_ENGINE_ID,
  SIGNALSMITH_ENGINE_ID,
} = ChromeUtils.importESModule("resource:///modules/AudioEngine.sys.mjs");

add_task(async function test_audio_engine_registry() {
  Assert.deepEqual(
    AudioEngine.engines.map(engine => engine.id),
    [FF_UPSTREAM_ENGINE_ID, SIGNALSMITH_ENGINE_ID, BUNGEE_ENGINE_ID]
  );
  Assert.ok(AudioEngine.isSupported(FF_UPSTREAM_ENGINE_ID));
  Assert.ok(!AudioEngine.isSupported("unknown"));
});

add_task(async function test_audio_engine_selection() {
  Services.prefs.clearUserPref(AUDIO_ENGINE_PREF);
  Assert.equal(AudioEngine.selected, FF_UPSTREAM_ENGINE_ID);

  Assert.ok(!AudioEngine.select(FF_UPSTREAM_ENGINE_ID));
  Assert.equal(
    Services.prefs.getStringPref(AUDIO_ENGINE_PREF),
    FF_UPSTREAM_ENGINE_ID
  );
  Assert.throws(
    () => AudioEngine.select("unknown"),
    /Unsupported audio engine/
  );

  Services.prefs.clearUserPref(AUDIO_ENGINE_PREF);
});

add_task(async function test_audio_engine_presets() {
  let engine = AudioEngine.getDefinition(SIGNALSMITH_ENGINE_ID);
  Assert.ok(engine.presets.length >= 3);
  Assert.ok(AudioEngine.setPreset(engine.id, "smooth"));
  Assert.equal(
    Services.prefs.getIntPref("media.audio.playbackrate.signalsmith_block_ms"),
    160
  );
  Assert.equal(AudioEngine.selectedPreset(engine), "smooth");

  let setting = engine.settings.find(candidate => candidate.id == "interval");
  Assert.ok(AudioEngine.setSetting(setting, 48));
  Assert.equal(AudioEngine.getSetting(setting), 48);
  Assert.equal(AudioEngine.selectedPreset(engine), undefined);

  AudioEngine.setPreset(engine.id, "balanced");
});
