# Loudness

## Values

| Value | Window | Use |
|---|---|---|
| Momentary loudness (M) | 400 ms | Fast feedback on the current sound |
| Short-term loudness (S) | 3 s | The main value for live adjustment |
| Integrated loudness (I) | From the stream start, gated | The value that services compare with their target |
| Loudness range (LRA) | From the stream start | How much the loudness changes |
| True peak (TP) | Each block, oversampled | Peaks between samples that can clip after the encoder |

The windows come from [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf) and [EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf). The algorithm is [ITU-R BS.1770](https://www.itu.int/rec/R-REC-BS.1770).

LRA is the difference between the 10th and the 95th percentiles of the S values. Two gates remove values before this calculation. The first gate removes values below -70 LUFS. The second gate removes values that are more than 20 LU below the mean of the remaining values.

## Targets

No single target applies to all streaming services. Most services do not publish a target for live streams. These values come from published standards and from the playback targets of on-demand services.

| Source | I | True peak max | Notes | Public source |
|---|---|---|---|---|
| YouTube | -14 LUFS | — | Not published by YouTube. Ian Shepherd (MeterPlugs) measured this value. YouTube turns down louder audio. It does not turn up quieter audio. | [MeterPlugs](https://www.meterplugs.com/blog/2019/09/18/youtube-changes-loudness-reference-to-14-lufs.html) |
| Spotify | -14 LUFS | -1 dBTP | For a master louder than -14 LUFS, Spotify recommends -2 dBTP. | [Spotify](https://support.spotify.com/artists/article/loudness-normalization/) |
| Apple Music | -16 LUFS | — | Not published by Apple. Ian Shepherd (MeterPlugs) measured this value. | [MeterPlugs](https://www.meterplugs.com/blog/2022/03/23/apple-switch-to-lufs.html) |
| Twitch | — | — | We found no official Twitch target. | — |
| AES TD1008 (2021), speech and mixed content | -18 LUFS | -1 dBTP | Standard for internet audio streaming. | [AES TD1008](https://www.aes.org/technical/documentDownloads.cfm?docID=731) |
| AES TD1008 (2021), music | -16 LUFS | -1 dBTP | Standard for internet audio streaming. | [AES TD1008](https://www.aes.org/technical/documentDownloads.cfm?docID=731) |
| EBU R 128 (broadcast) | -23 LUFS | -1 dBTP | TV standard. Too quiet for most streams. | [EBU R 128](https://tech.ebu.ch/publications/r128) |

## Presets

The operator selects a preset in the settings. `src/presets.c` has the same table.

| Preset | I target |
|---|---|
| YouTube (default) | -14 LUFS |
| Spotify | -14 LUFS |
| Apple Music | -16 LUFS |
| AES TD1008 speech | -18 LUFS |
| AES TD1008 music | -16 LUFS |
| EBU R 128 | -23 LUFS |
| Custom | -30 to -5 LUFS |

Twitch has no preset, because Twitch publishes no value.

The default is -14 LUFS, ±1 LU, with a -1 dBTP ceiling. Spotify publishes -14 LUFS, and third-party measurements give the same value for YouTube.

The true-peak ceiling is a separate setting, from -3 to 0 dBTP. At audio bitrates below 128 kbps, the AAC encoder can make peaks higher. For these bitrates, set the ceiling to -2 dBTP.

## Status

The dock status follows S. The full window also shows the status of I.

| Difference from the target | Color | Label |
|---|---|---|
| No value | grey | No signal |
| ±1 LU or less | green | On target |
| 1 LU to 3 LU above | amber | A bit loud |
| More than 3 LU above | red | Too loud |
| 1 LU to 3 LU below | amber | A bit quiet |
| More than 3 LU below | red | Too quiet |

TP is red above the ceiling, and green at or below the ceiling.
