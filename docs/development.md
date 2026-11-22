# Development

## Toolchain

The project uses the official [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate). The template supplies the CMake presets, the build scripts, and the GitHub Actions workflows.

| Platform | Tools |
|---|---|
| macOS | Xcode 16 or later, CMake 3.28 or later |
| Windows | Visual Studio 2022, Windows SDK 10.0.20348 or later, CMake 3.28 or later |
| Ubuntu 24.04 | GCC, Ninja, CMake 3.28 or later, Qt 6 and OBS development packages |

CMake downloads the OBS sources and the dependencies that `buildspec.json` lists. It also downloads libebur128 1.2.6.

## Build

```sh
cmake --preset macos
cmake --build --preset macos
```

On the other platforms, use `windows-x64` or `ubuntu-x86_64` in place of `macos`.

## Test

The build makes the `airmeter-tests` program. It tests the meter with the EBU Tech 3341 and Tech 3342 test signals, and it tests the status rules, the presets, and the history.

```sh
ctest --test-dir build_macos -C RelWithDebInfo --output-on-failure
```

On Windows, the build folder is `build_x64`. On Ubuntu, it is `build_x86_64`.

## Format

- C and C++: clang-format 19 with `.clang-format`.
- CMake: gersemi with `.gersemirc`.

## CI

The GitHub Actions workflows check the format, build on the three platforms, and run the tests. A tag such as `1.2.3` makes a draft release.

The workflows are off on GitHub. To turn them on:

```sh
gh workflow enable push.yaml
gh workflow enable pr-pull.yaml
gh workflow enable dispatch.yaml
```

The `on:` blocks in `push.yaml` and `pr-pull.yaml` have only `workflow_dispatch`. Add the branch and tag triggers back before you turn the workflows on.

## How it works

1. `obs_module_load()` calls `obs_add_raw_audio_callback()` for the selected mix.
2. OBS calls the callback on the audio thread with each block of float audio.
3. The callback sends the block to [libebur128](https://github.com/jiixyj/libebur128), which calculates the BS.1770 values.
4. A Qt timer on the UI thread reads the values 10 times each second and updates the dock.

The audio callback does no UI work and does not allocate memory. It holds a lock only for one libebur128 call. This keeps the stream audio free of gaps.

| File | Contents |
|---|---|
| `src/loudness.c` | libebur128 wrapper. No OBS calls. |
| `src/meter.c` | The OBS audio callback and the channel map. |
| `src/presets.c` | The target presets and the status rules. |
| `src/history.hpp` | The history buffers for the graphs. |
| `src/ui.cpp` | The dock, the full window, and the settings dialog. |
| `tests/airmeter-tests.cpp` | The measurement tests. |

## References

- [ITU-R BS.1770](https://www.itu.int/rec/R-REC-BS.1770): loudness and true-peak algorithm.
- [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf): loudness meter.
- [EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf): loudness range.
- [OBS plugin documentation](https://docs.obsproject.com/plugins).
