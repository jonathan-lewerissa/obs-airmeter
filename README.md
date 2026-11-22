# obs-airmeter

obs-airmeter is an OBS Studio plugin. It shows the loudness of the audio that OBS sends to the streaming service. The operator can then keep the stream near a known loudness target.

Status: planning. No code exists yet.

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
| YouTube | -14 LUFS | — | YouTube does not publish this value. Third-party measurements give it. YouTube reduces the level of louder audio on playback. It does not increase the level of quieter audio. | None found |
| Spotify | -14 LUFS | -1 dBTP | Reference value for music content. If the master is louder than -14 LUFS, Spotify recommends -2 dBTP. | [Spotify loudness normalization](https://support.spotify.com/artists/article/loudness-normalization/) |
| Apple Music | -16 LUFS | — | Apple does not publish this value. Third-party measurements give it. | None found |
| Twitch | Not published | — | We found no official Twitch loudness target. | — |
| AES TD1008 (2021), speech and mixed content | -18 LUFS | -1 dBTP | Standard for internet audio streaming. | AES TD1008, from aes.org |
| AES TD1008 (2021), music | -16 LUFS | -1 dBTP | Standard for internet audio streaming. | AES TD1008, from aes.org |
| EBU R 128 (broadcast) | -23 LUFS | -1 dBTP | TV standard. Too quiet for most streams. | [EBU R 128](https://tech.ebu.ch/publications/r128) |

### Default target

The plugin uses these defaults:

- Integrated loudness target: **-14 LUFS**.
- Tolerance: **±1 LU** for integrated loudness.
- True peak ceiling: **-1 dBTP**.

Reason: -14 LUFS is the published Spotify target. YouTube does not publish a target, but third-party measurements also give -14 LUFS. The stream is also not too quiet on Twitch.

The plugin will also include these presets:

| Preset | Target | Use |
|---|---|---|
| Streaming (default) | -14 LUFS | Games, music, mixed content |
| Speech | -16 LUFS | Talk shows, podcasts, tutorials |
| AES speech | -18 LUFS | Speech content that follows AES TD1008 |
| Custom | Operator value | Any other target |

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

The template downloads the OBS sources and dependencies that `buildspec.json` lists. Read the template README for the toolchain versions (Xcode, Visual Studio, GCC, CMake).

## Install

Copy the built plugin to the OBS plugin folder for your platform:

| Platform | Folder |
|---|---|
| Windows | `C:\ProgramData\obs-studio\plugins\obs-airmeter\` |
| macOS | `~/Library/Application Support/obs-studio/plugins/obs-airmeter.plugin` |
| Linux | `~/.config/obs-studio/plugins/obs-airmeter/` |

Restart OBS. Open the dock from the **Docks** menu.

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
