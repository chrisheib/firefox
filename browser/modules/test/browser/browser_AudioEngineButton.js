/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

add_task(async function test_audio_engine_toolbar_button() {
  let win = window;
  let button = win.document.getElementById("audio-engine-button");
  let popup = win.document.getElementById("audio-engine-popup");

  Assert.ok(button);
  Assert.ok(BrowserTestUtils.isVisible(button));

  let shown = BrowserTestUtils.waitForEvent(popup, "popupshown");
  EventUtils.synthesizeMouseAtCenter(button, {}, win);
  await shown;

  let engines = popup.querySelectorAll(".audio-engine-menuitem");
  Assert.equal(engines.length, 3);
  Assert.equal(engines[0].value, "ff-upstream");
  Assert.equal(engines[0].getAttribute("checked"), "true");
  Assert.equal(
    popup.querySelectorAll(".audio-engine-preset-menuitem").length,
    12
  );
  Assert.equal(
    popup.querySelectorAll(".audio-engine-setting-menuitem").length,
    37
  );

  popup.hidePopup();
});
