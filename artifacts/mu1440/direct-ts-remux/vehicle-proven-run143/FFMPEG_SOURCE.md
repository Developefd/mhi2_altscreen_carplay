# FFmpeg corresponding source for direct-ts-remux

The public `direct-ts-remux` developer binary statically links a minimal FFmpeg 6.1.5 library build.

## Exact source

- Project: FFmpeg
- Version: 6.1.5
- Archive: `https://ffmpeg.org/releases/ffmpeg-6.1.5.tar.gz`
- SHA-256: `b8c8e926b948c14df1264cd0beac1c773df9170ac9cac97bdf1275cd3d385902`
- License: LGPL-2.1-or-later for the enabled/default subset
- Local license copy: `LICENSES/LGPL-2.1-or-later.txt`

The build does not enable FFmpeg's optional GPL or nonfree components.

## Enabled subset

The reproducible build enables only what this remuxer requires:

- H.264 demuxer
- H.264 parser
- MPEG-TS muxer
- file protocol
- TCP protocol
- libavformat
- libavcodec
- libavutil

The complete configure/build invocation is executable documentation in:

`tools/build_direct_ts_remux.sh`

Because the project's own remuxer source and the complete library build procedure are published,
a developer can replace/rebuild the FFmpeg library set and relink the executable instead of depending
on an opaque prebuilt multimedia library.
