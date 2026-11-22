# obs-airmeter

An OBS Studio plugin that shows the loudness of your stream audio in LUFS. Use it to keep a live stream near the target of YouTube, Spotify, or another service.

Status: in development. There is no release yet.

## Why

The OBS mixer shows peak levels. A peak meter tells you if the audio clips. It does not tell you how loud the audio sounds. Streaming services measure loudness in LUFS, and some of them turn down loud audio on playback. OBS has no LUFS meter.

obs-airmeter only measures. It does not change the audio. To correct the level, use the OBS gain, compressor, or limiter filters.

## Features

- Momentary (M), short-term (S) and integrated (I) loudness, loudness range (LRA) and true peak (TP), as EBU Tech 3341 and Tech 3342 specify.
- Target presets: YouTube, Spotify, Apple Music, AES TD1008, EBU R 128, and a custom value. The default is -14 LUFS.
- A status color that shows if the stream is on target, too loud, or too quiet.
- A compact dock, and a full window with history graphs.
- Any of the 6 OBS audio tracks.

## Requirements

- OBS Studio 31.1 or later, 64-bit.
- macOS 12 or later, Windows 10 or later, or Ubuntu 24.04 or later.

## Install

Copy the plugin to the OBS plugin folder:

| Platform | Folder |
|---|---|
| macOS | `~/Library/Application Support/obs-studio/plugins/obs-airmeter.plugin` |
| Windows | `C:\ProgramData\obs-studio\plugins\obs-airmeter\` |
| Linux | `~/.config/obs-studio/plugins/obs-airmeter/` |

Restart OBS. Open the dock from the **Docks** menu.

## Use

- The dock shows S, the status, M, I, and the highest true peak.
- Click ⚙ to select the track, the preset, the true-peak ceiling, and the window layout.
- Click ⤢ to open the full window. Layout A shows one graph of S and M. Layout B shows graphs of S, I, and true peak.
- I, LRA, the highest true peak, and the graphs reset when the stream starts, when you click **Reset**, and when you select a different track.

## Documentation

- [Loudness](docs/loudness.md): the values, the targets and their sources, the presets, and the status rules.
- [Development](docs/development.md): build, test, CI, and how the plugin works.
- [AGENTS.md](AGENTS.md): rules for AI agents that work on this project.

## Other plugins

[Loudness Dock](https://github.com/norihiro/obs-loudness-dock) and [Master Level Meter](https://obsproject.com/forum/resources/master-level-meter.2213/) also show LUFS in OBS. obs-airmeter adds a target status with presets.

## License

GPL-2.0-or-later. OBS Studio uses this license, and plugins that link to libobs must use a compatible license.
