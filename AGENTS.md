# AGENTS.md

Instructions for AI agents that work on obs-airmeter. Read README.md first for the product. Read docs/loudness.md for the loudness targets.

## Project

obs-airmeter is an OBS Studio plugin. It measures the loudness of the stream audio mix. It shows the values in a dock.

- Language: C for the module and the audio code. C++ with Qt 6 for the dock.
- Build system: CMake, from the official obs-plugintemplate.
- Loudness library: libebur128.
- License: GPL-2.0-or-later.

## Glossary

| Term | Meaning |
|---|---|
| LUFS | Loudness Units relative to Full Scale. Absolute loudness. |
| LU | Loudness Unit. A difference between two LUFS values. 1 LU = 1 dB. |
| dBTP | Decibels True Peak. Peak level after oversampling. |
| M | Momentary loudness. 400 ms window. |
| S | Short-term loudness. 3 s window. |
| I | Integrated loudness. Gated, from the measurement start. |
| LRA | Loudness range, in LU. |
| Mix | One of the 6 OBS audio tracks. Index 0 to 5. |

Use these terms with these meanings only.

## Rules

### Measurement

1. Do not change the audio. The plugin only reads audio.
2. Use libebur128 for all loudness values. Do not write a K-weighting filter.
3. Use `EBUR128_MODE_M | EBUR128_MODE_S | EBUR128_MODE_I | EBUR128_MODE_LRA | EBUR128_MODE_TRUE_PEAK | EBUR128_MODE_HISTOGRAM`. Histogram mode keeps `ebur128_add_frames_float()` free of memory allocation. It also keeps the I and LRA reads fast.
4. Report true peak, not sample peak.
5. Reset the integrated loudness (I) and the LRA when the stream starts.
6. Show "—" when a value is not available. Do not show `-inf` or `-HUGE_VAL`.

### Threads

1. OBS calls the raw audio callback on the audio thread.
2. In the audio callback, do not allocate memory.
3. In the audio callback, do not call Qt.
4. In the audio callback, do not write logs.
5. Do not hold a lock for longer than one `ebur128_add_frames_float()` call.
6. Update the dock only from the Qt UI thread. Use a `QTimer` with a 100 ms interval.

### OBS API

1. Get audio with `obs_add_raw_audio_callback()`. Remove it with `obs_remove_raw_audio_callback()` in `obs_module_unload()`.
2. Use mix 0 (track 1) as the default. Let the operator select another track in the settings.
3. Add the dock with `obs_frontend_add_dock_by_id()`.
4. Get the sample rate and channel count from `obs_get_audio_info()`. Do not assume 48 kHz or stereo.
5. Put all user-visible strings in `data/locale/en-US.ini`. Get them with `obs_module_text()`.
6. Before you use an OBS function, find it in the OBS headers for the version in `buildspec.json`. Do not use a function that is not in these headers.

### Targets

1. The default target is -14 LUFS integrated, ±1 LU, with a -1 dBTP true-peak ceiling.
2. Keep the presets in one table in the source. The table must agree with the presets table in docs/loudness.md.
3. If you change a target value, change docs/loudness.md in the same commit.
4. Do not add a target without a public source. Put the source URL in a comment next to the value. If the service does not publish the value, you can use a third-party measurement. Then docs/loudness.md and the code comment must both say "Not published by <service>" and name the source of the measurement.

### Scope

Do not add these features unless the user asks:

- Automatic gain or loudness correction.
- Log files or loudness reports.
- Network output.
- Meters for each source.

## Build and test

macOS:

```sh
cmake --preset macos
cmake --build --preset macos
```

Use `windows-x64` or `ubuntu-x86_64` in place of `macos` on the other platforms.

Before you report a change as complete:

1. Build the plugin with no warnings.
2. Run the measurement tests: `ctest --test-dir build_macos -C RelWithDebInfo --output-on-failure`.
3. Load the plugin in OBS. Check the OBS log for errors from the plugin.

### Measurement tests

Test the meter with the EBU Tech 3341 and Tech 3342 test signals. The values must be in the tolerances that these documents give. Do not change a test tolerance to make a test pass.

## Code style

- Format C and C++ with the `.clang-format` file from the plugin template.
- Format CMake with the `.gersemirc` file from the plugin template.
- Use the `obs-airmeter` log prefix with `blog()`.
- Do not write a comment that the code already explains.
- Write each comment in ASD-STE100 style. Use short sentences, active voice, and one meaning for each word.

## Documents

- Write README.md, AGENTS.md, and the files in docs/ in ASD-STE100 style. Use short sentences, active voice, and one meaning for each word.
- Keep README.md short and for operators. Put details for contributors in docs/. Keep AGENTS.md for agents.
