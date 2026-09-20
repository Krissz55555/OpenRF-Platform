#pragma once

// Runs the OpenRF boot indication on the onboard addressable RGB LED:
// 5 red flashes at 0.5 s cadence, then 1 s solid green, then off.
// This is intentionally blocking and is called once at the very beginning
// of setup(), before normal OpenRF services start.
void statusLedBootSequence();
