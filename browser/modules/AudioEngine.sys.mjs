/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

export const AUDIO_ENGINE_PREF = "media.audio_engine";
export const FF_UPSTREAM_ENGINE_ID = "ff-upstream";
export const SIGNALSMITH_ENGINE_ID = "signalsmith-stretch";
export const BUNGEE_ENGINE_ID = "bungee";

const engines = Object.freeze([
  Object.freeze({
    id: FF_UPSTREAM_ENGINE_ID,
    labelL10nId: "audio-engine-ff-upstream",
    settingsL10nId: "audio-engine-settings-ff-upstream",
    presets: [
      {
        id: "firefox-default",
        labelL10nId: "audio-engine-preset-firefox-default",
        values: {
          "media.audio.playbackrate.soundtouch_sequence_ms": 10,
          "media.audio.playbackrate.soundtouch_seekwindow_ms": 15,
          "media.audio.playbackrate.soundtouch_overlap_ms": 8,
        },
      },
      {
        id: "speech",
        labelL10nId: "audio-engine-preset-speech",
        values: {
          "media.audio.playbackrate.soundtouch_sequence_ms": 10,
          "media.audio.playbackrate.soundtouch_seekwindow_ms": 12,
          "media.audio.playbackrate.soundtouch_overlap_ms": 6,
        },
      },
      {
        id: "balanced",
        labelL10nId: "audio-engine-preset-balanced",
        values: {
          "media.audio.playbackrate.soundtouch_sequence_ms": 25,
          "media.audio.playbackrate.soundtouch_seekwindow_ms": 15,
          "media.audio.playbackrate.soundtouch_overlap_ms": 8,
        },
      },
      {
        id: "music",
        labelL10nId: "audio-engine-preset-music",
        values: {
          "media.audio.playbackrate.soundtouch_sequence_ms": 40,
          "media.audio.playbackrate.soundtouch_seekwindow_ms": 20,
          "media.audio.playbackrate.soundtouch_overlap_ms": 10,
        },
      },
      {
        id: "smooth",
        labelL10nId: "audio-engine-preset-smooth",
        values: {
          "media.audio.playbackrate.soundtouch_sequence_ms": 60,
          "media.audio.playbackrate.soundtouch_seekwindow_ms": 25,
          "media.audio.playbackrate.soundtouch_overlap_ms": 12,
        },
      },
    ],
    settings: [
      {
        id: "sequence",
        pref: "media.audio.playbackrate.soundtouch_sequence_ms",
        labelL10nId: "audio-engine-setting-soundtouch-sequence",
        valueL10nId: "audio-engine-value-milliseconds",
        type: "int",
        values: [10, 20, 25, 40, 60],
      },
      {
        id: "seek-window",
        pref: "media.audio.playbackrate.soundtouch_seekwindow_ms",
        labelL10nId: "audio-engine-setting-soundtouch-seek-window",
        valueL10nId: "audio-engine-value-milliseconds",
        type: "int",
        values: [10, 12, 15, 20, 25],
      },
      {
        id: "overlap",
        pref: "media.audio.playbackrate.soundtouch_overlap_ms",
        labelL10nId: "audio-engine-setting-soundtouch-overlap",
        valueL10nId: "audio-engine-value-milliseconds",
        type: "int",
        values: [4, 6, 8, 10, 12],
      },
    ],
  }),
  Object.freeze({
    id: SIGNALSMITH_ENGINE_ID,
    labelL10nId: "audio-engine-signalsmith",
    settingsL10nId: "audio-engine-settings-signalsmith",
    presets: [
      {
        id: "low-latency",
        labelL10nId: "audio-engine-preset-low-latency",
        values: {
          "media.audio.playbackrate.signalsmith_block_ms": 80,
          "media.audio.playbackrate.signalsmith_interval_ms": 20,
          "media.audio.playbackrate.signalsmith_split_computation": false,
        },
      },
      {
        id: "balanced",
        labelL10nId: "audio-engine-preset-balanced",
        values: {
          "media.audio.playbackrate.signalsmith_block_ms": 120,
          "media.audio.playbackrate.signalsmith_interval_ms": 30,
          "media.audio.playbackrate.signalsmith_split_computation": false,
        },
      },
      {
        id: "smooth",
        labelL10nId: "audio-engine-preset-smooth",
        values: {
          "media.audio.playbackrate.signalsmith_block_ms": 160,
          "media.audio.playbackrate.signalsmith_interval_ms": 40,
          "media.audio.playbackrate.signalsmith_split_computation": false,
        },
      },
      {
        id: "low-cpu",
        labelL10nId: "audio-engine-preset-low-cpu",
        values: {
          "media.audio.playbackrate.signalsmith_block_ms": 100,
          "media.audio.playbackrate.signalsmith_interval_ms": 40,
          "media.audio.playbackrate.signalsmith_split_computation": true,
        },
      },
    ],
    settings: [
      {
        id: "block",
        pref: "media.audio.playbackrate.signalsmith_block_ms",
        labelL10nId: "audio-engine-setting-signalsmith-block",
        valueL10nId: "audio-engine-value-milliseconds",
        type: "int",
        values: [64, 80, 100, 120, 160, 192],
      },
      {
        id: "interval",
        pref: "media.audio.playbackrate.signalsmith_interval_ms",
        labelL10nId: "audio-engine-setting-signalsmith-interval",
        valueL10nId: "audio-engine-value-milliseconds",
        type: "int",
        values: [16, 20, 24, 30, 40, 48],
      },
      {
        id: "split-computation",
        pref: "media.audio.playbackrate.signalsmith_split_computation",
        labelL10nId: "audio-engine-setting-signalsmith-split",
        type: "bool",
        values: [false, true],
      },
    ],
  }),
  Object.freeze({
    id: BUNGEE_ENGINE_ID,
    labelL10nId: "audio-engine-bungee",
    settingsL10nId: "audio-engine-settings-bungee",
    presets: [
      {
        id: "low-latency",
        labelL10nId: "audio-engine-preset-low-latency",
        values: {
          "media.audio.playbackrate.bungee_hop_adjust": -1,
          "media.audio.playbackrate.bungee_chunk_frames": 1024,
        },
      },
      {
        id: "balanced",
        labelL10nId: "audio-engine-preset-balanced",
        values: {
          "media.audio.playbackrate.bungee_hop_adjust": 0,
          "media.audio.playbackrate.bungee_chunk_frames": 2048,
        },
      },
      {
        id: "dense-music",
        labelL10nId: "audio-engine-preset-dense-music",
        values: {
          "media.audio.playbackrate.bungee_hop_adjust": 1,
          "media.audio.playbackrate.bungee_chunk_frames": 4096,
        },
      },
    ],
    settings: [
      {
        id: "hop-adjust",
        pref: "media.audio.playbackrate.bungee_hop_adjust",
        labelL10nId: "audio-engine-setting-bungee-hop",
        valueL10nId: "audio-engine-value-number",
        type: "int",
        values: [-1, 0, 1],
      },
      {
        id: "chunk-frames",
        pref: "media.audio.playbackrate.bungee_chunk_frames",
        labelL10nId: "audio-engine-setting-bungee-chunk",
        valueL10nId: "audio-engine-value-frames",
        type: "int",
        values: [512, 1024, 2048, 4096, 8192],
      },
    ],
  }),
]);

const enginesById = new Map(engines.map(engine => [engine.id, engine]));

export var AudioEngine = Object.freeze({
  engines,

  get selected() {
    let id = Services.prefs.getStringPref(
      AUDIO_ENGINE_PREF,
      FF_UPSTREAM_ENGINE_ID
    );
    return enginesById.has(id) ? id : FF_UPSTREAM_ENGINE_ID;
  },

  get definition() {
    return enginesById.get(this.selected);
  },

  isSupported(id) {
    return enginesById.has(id);
  },

  getDefinition(id) {
    return enginesById.get(id);
  },

  getSetting(setting) {
    if (setting.type == "bool") {
      return Services.prefs.getBoolPref(setting.pref);
    }
    return Services.prefs.getIntPref(setting.pref);
  },

  setSetting(setting, value) {
    let oldValue = this.getSetting(setting);
    if (setting.type == "bool") {
      Services.prefs.setBoolPref(setting.pref, Boolean(value));
    } else {
      Services.prefs.setIntPref(setting.pref, Number(value));
    }
    return oldValue != value;
  },

  setPreset(engineId, presetId) {
    let engine = enginesById.get(engineId);
    let preset = engine?.presets.find(candidate => candidate.id == presetId);
    if (!preset) {
      throw new RangeError(`Unsupported audio engine preset: ${presetId}`);
    }
    let changed = false;
    for (let [pref, value] of Object.entries(preset.values)) {
      if (typeof value == "boolean") {
        changed = Services.prefs.getBoolPref(pref) != value || changed;
        Services.prefs.setBoolPref(pref, value);
      } else {
        changed = Services.prefs.getIntPref(pref) != value || changed;
        Services.prefs.setIntPref(pref, value);
      }
    }
    return changed;
  },

  selectedPreset(engine) {
    return engine.presets.find(preset =>
      Object.entries(preset.values).every(([pref, value]) =>
        typeof value == "boolean"
          ? Services.prefs.getBoolPref(pref) == value
          : Services.prefs.getIntPref(pref) == value
      )
    )?.id;
  },

  select(id) {
    if (!this.isSupported(id)) {
      throw new RangeError(`Unsupported audio engine: ${id}`);
    }
    let changed = this.selected != id;
    Services.prefs.setStringPref(AUDIO_ENGINE_PREF, id);
    return changed;
  },
});
