/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

export const AUDIO_ENGINE_PREF = "media.audio_engine";
export const FF_UPSTREAM_ENGINE_ID = "ff-upstream";

const engines = Object.freeze([
  Object.freeze({
    id: FF_UPSTREAM_ENGINE_ID,
    labelL10nId: "audio-engine-ff-upstream",
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

  select(id) {
    if (!this.isSupported(id)) {
      throw new RangeError(`Unsupported audio engine: ${id}`);
    }
    let changed = this.selected != id;
    Services.prefs.setStringPref(AUDIO_ENGINE_PREF, id);
    return changed;
  },
});
