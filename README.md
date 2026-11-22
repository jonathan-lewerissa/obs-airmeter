# obs-airmeter

obs-airmeter is an OBS Studio plugin. It shows the loudness of the audio that OBS sends to the streaming service. The operator can then keep the stream near a known loudness target.

Status: in development. The plugin is not released yet.

## The problem

The OBS mixer shows sample peak levels in dBFS. A peak meter tells you if the audio clips. It does not tell you how loud the audio sounds to a listener. Two streams with the same peak level can have very different loudness.

Streaming services measure loudness in LUFS (Loudness Units relative to Full Scale). Some services reduce the level of loud audio on playback. OBS has no LUFS meter, so the operator cannot see this value during a live stream.

## What the plugin does

1. It reads the audio mix that goes to the stream output.
2. It measures loudness with the ITU-R BS.1770 algorithm.
3. It shows the values in a dock in the OBS window.
4. It compares the values with a target and shows a status color.

The plugin only measures. It does not change the audio. To correct the level, use the OBS gain, compressor, or limiter filters.

### Values on the meter

| Value | Window | Use |
|---|---|---|
| Momentary loudness (M) | 400 ms | Fast feedback on the current sound |
| Short-term loudness (S) | 3 s | The main value for live adjustment |
| Integrated loudness (I) | From the stream start, gated | The value that services compare with their target |
| Loudness range (LRA) | Whole stream | How much the loudness changes |
| True peak (TP) | Per block, oversampled | Peaks between samples that can clip after the encoder |

These windows come from EBU Tech 3341 (meter) and EBU Tech 3342 (loudness range).

LRA is the difference between the 10th and the 95th percentiles of the short-term loudness values. Before this calculation, two gates remove values. The first gate removes values below -70 LUFS. The second gate removes values that are more than 20 LU below the mean of the remaining values. Source: [EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf).

## Loudness targets

No single loudness target applies to all streaming services. Most services do not publish a target for live streams. The values below come from published standards and from the playback targets of on-demand services.

| Source | Integrated loudness | True peak max | Notes | Public source |
|---|---|---|---|---|
| YouTube | -14 LUFS | — | Not published by YouTube. Ian Shepherd (MeterPlugs) measured this value. YouTube reduces the level of louder audio on playback. It does not increase the level of quieter audio. | [MeterPlugs measurement](https://www.meterplugs.com/blog/2019/09/18/youtube-changes-loudness-reference-to-14-lufs.html) |
| Spotify | -14 LUFS | -1 dBTP | Reference value for music content. If the master is louder than -14 LUFS, Spotify recommends -2 dBTP. | [Spotify loudness normalization](https://support.spotify.com/artists/article/loudness-normalization/) |
| Apple Music | -16 LUFS | — | Not published by Apple. Ian Shepherd (MeterPlugs) measured this value. | [MeterPlugs measurement](https://www.meterplugs.com/blog/2022/03/23/apple-switch-to-lufs.html) |
| Twitch | Not published | — | We found no official Twitch loudness target. | — |
| AES TD1008 (2021), speech and mixed content | -18 LUFS | -1 dBTP | Standard for internet audio streaming. | [AES TD1008](https://www.aes.org/technical/documentDownloads.cfm?docID=731) |
| AES TD1008 (2021), music | -16 LUFS | -1 dBTP | Standard for internet audio streaming. | [AES TD1008](https://www.aes.org/technical/documentDownloads.cfm?docID=731) |
| EBU R 128 (broadcast) | -23 LUFS | -1 dBTP | TV standard. Too quiet for most streams. | [EBU R 128](https://tech.ebu.ch/publications/r128) |

### Default target

The plugin uses these defaults:

- Integrated loudness target: **-14 LUFS**.
- Tolerance: **±1 LU** for integrated loudness.
- True peak ceiling: **-1 dBTP**.

Reason: -14 LUFS is the published Spotify target. YouTube does not publish a target, but third-party measurements also give -14 LUFS. The stream is also not too quiet on Twitch.

### Presets

Each preset has the name of the target that it is for. The operator selects a preset in the settings.

| Preset | Integrated loudness | Source |
|---|---|---|
| YouTube (default) | -14 LUFS | Not published by YouTube. [MeterPlugs measurement](https://www.meterplugs.com/blog/2019/09/18/youtube-changes-loudness-reference-to-14-lufs.html) |
| Spotify | -14 LUFS | [Spotify](https://support.spotify.com/artists/article/loudness-normalization/) |
| Apple Music | -16 LUFS | Not published by Apple. [MeterPlugs measurement](https://www.meterplugs.com/blog/2022/03/23/apple-switch-to-lufs.html) |
| AES TD1008 speech | -18 LUFS | [AES TD1008](https://www.aes.org/technical/documentDownloads.cfm?docID=731) |
| AES TD1008 music | -16 LUFS | [AES TD1008](https://www.aes.org/technical/documentDownloads.cfm?docID=731) |
| EBU R 128 | -23 LUFS | [EBU R 128](https://tech.ebu.ch/publications/r128) |
| Custom | -30 to -5 LUFS, operator value | — |

The true-peak ceiling is a separate setting from -3 to 0 dBTP. The default is -1 dBTP. Twitch has no preset, because Twitch publishes no value.

### Status

The status follows S in the dock. It also follows the S and I values in the full window.

| Difference from the target | Color | Label |
|---|---|---|
| No value | grey | No signal |
| ±1 LU or less | green | On target |
| 1 LU to 3 LU above | amber | A bit loud |
| More than 3 LU above | red | Too loud |
| 1 LU to 3 LU below | amber | A bit quiet |
| More than 3 LU below | red | Too quiet |

The TP value is red when it is above the ceiling. It is green when it is at or below the ceiling.

At low audio bitrates, the AAC encoder can make peaks higher. For audio bitrates below 128 kbps, set the true peak ceiling to -2 dBTP.

## Requirements

- OBS Studio 30 or later, 64-bit.
- Windows 10 or later, macOS 12 or later, or Linux (Ubuntu 22.04 or later).

These versions are a plan. The CI matrix will set the final values.

## Build

The project uses the official [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate). The template supplies CMake presets, build scripts, and GitHub Actions workflows for the three platforms.

macOS:

```sh
cmake --preset macos
cmake --build --preset macos
```

Windows:

```sh
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Linux:

```sh
cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64
```

The template downloads the OBS sources and dependencies that `buildspec.json` lists. CMake also downloads libebur128 1.2.6.

### Tests

The build makes the `airmeter-tests` program. It tests the meter with the EBU Tech 3341 and Tech 3342 test signals. macOS:

```sh
ctest --test-dir build_macos -C RelWithDebInfo --output-on-failure
``` Read the template README for the toolchain versions (Xcode, Visual Studio, GCC, CMake).

## Install

Copy the built plugin to the OBS plugin folder for your platform:

| Platform | Folder |
|---|---|
| Windows | `C:\ProgramData\obs-studio\plugins\obs-airmeter\` |
| macOS | `~/Library/Application Support/obs-studio/plugins/obs-airmeter.plugin` |
| Linux | `~/.config/obs-studio/plugins/obs-airmeter/` |

Restart OBS. Open the dock from the **Docks** menu.

## Use

- The dock shows S, the status, M, I and the highest true peak.
- Click ⚙ to open the settings: the track, the preset, the custom target, the true-peak ceiling and the window layout.
- Click ⤢ to open the full window. Layout A shows five values and one graph of S and M. Layout B shows three graphs (S, I and true peak) and the five values.
- On Windows, use Alt-Tab to go to the full window. On macOS, use Cmd-` or the ⤢ button.
- I, LRA, TP max and the graphs reset when the stream starts, when you click **Reset**, and when you select a different track.

## How it works

1. In `obs_module_load()`, the plugin calls `obs_add_raw_audio_callback()` for the mix that goes to the stream.
2. OBS calls the callback on the audio thread with each block of float audio.
3. The callback sends the block to [libebur128](https://github.com/jiixyj/libebur128). libebur128 calculates the BS.1770 values.
4. A Qt timer on the UI thread reads the latest values 10 times each second and updates the dock.

The audio callback does no UI work. It holds a lock only for one libebur128 call. This keeps the stream audio free of gaps.

## Other plugins

[Loudness Dock](https://github.com/norihiro/obs-loudness-dock) and [Master Level Meter](https://obsproject.com/forum/resources/master-level-meter.2213/) also show LUFS in OBS. obs-airmeter adds a stream-target status with presets.

## References

- [ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770): loudness and true-peak algorithm.
- [EBU R 128](https://tech.ebu.ch/publications/r128): broadcast loudness standard.
- [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf): loudness meter definition.
- [EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf): loudness range.
- AES TD1008: loudness for internet audio streaming and on-demand distribution. Get it from aes.org.
- [Spotify loudness normalization](https://support.spotify.com/artists/article/loudness-normalization/).
- [OBS plugin documentation](https://docs.obsproject.com/plugins).
- [OBS plugin template](https://github.com/obsproject/obs-plugintemplate).

## License

GPL-2.0-or-later. OBS Studio uses this license, and plugins that link to libobs must use a compatible license.
