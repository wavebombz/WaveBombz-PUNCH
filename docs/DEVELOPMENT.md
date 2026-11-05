# Development roadmap

Initial JUCE 8 / C++17 / CMake VST3 foundation.

Planned engineering passes:
1. Compile on Windows with GitHub Actions and resolve compiler/API issues.
2. Audit oversampling latency and host delay compensation; implement safe mode switching.
3. Add factory presets and user preset management.
4. Improve metering with peak hold and RMS ballistics; add waveform history.
5. Add parameter smoothing for all continuous DSP controls and calibrate transient detection.
6. Validate mono/stereo, sample rates, block sizes, automation, save/restore and FL Studio loading.

Real-time requirements: allocate DSP engines and scratch buffers in prepareToPlay, avoid file I/O and locks in processBlock, and verify buffer-size handling. This initial foundation is not yet production-validated.
