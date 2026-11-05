# Development roadmap

## Implemented

- JUCE 8 / C++17 / CMake VST3 project foundation.
- Windows x64 CI build and VST3 artifact publishing.
- APVTS parameters, host automation hooks, state save/restore and eight factory starting presets.
- Attack, Sustain, Punch, Mix, Input and Output controls with smoothing.
- Oversized host blocks are processed in bounded chunks using a scratch buffer allocated in `prepareToPlay`.
- Peak and RMS input/output measurements are exposed to the editor.
- Oversampling modes (Off / 2x / 4x / 8x) are initialized before audio processing; selected oversampler is reset when the active mode changes.
- Oversampling latency is read from the active JUCE oversampler and reported with `setLatencySamples`.
- Host bypass callback is transparent and does not run the effect DSP.

## Remaining engineering work

1. Verify latency reporting and mode changes in real hosts (especially FL Studio); hosts differ in how they handle dynamic plugin latency changes during playback.
2. Add automated DSP tests for impulse response, null/dry path, clipping, transient response, mono/stereo and variable block sizes.
3. Calibrate transient detection and gain law using representative kick, snare, percussion, bass and drum-bus material.
4. Add peak hold / RMS ballistics and optional waveform history to the UI.
5. Add user preset save/load with file operations strictly outside the audio callback.
6. Validate sample rates, buffer sizes, automation, state restore, bypass and plugin scanning in FL Studio.

## Real-time safety notes

- DSP engines and scratch storage are created in `prepareToPlay`, not in `processBlock`.
- Audio processing handles host blocks larger than the prepared scratch capacity by splitting them into bounded chunks.
- No file I/O or explicit mutex locking is performed in the audio callback.
- This project has passed the Windows CI compile, but it is not yet production-validated. A successful compile does not replace listening tests or host compatibility tests.
