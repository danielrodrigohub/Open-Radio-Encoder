<p align="center">
  <img src="img_obe.png" alt="Open Radio Encoder" width="400"/>
</p>

<h1 align="center">Open Radio Encoder</h1>

<p align="center">
  <strong>Multi-station internet radio streaming encoder</strong><br/>
  Made in Chile with Love!
</p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-blue" alt="Platforms"/>
  <img src="https://img.shields.io/badge/language-C%2B%2B17-blue" alt="Language"/>
  <img src="https://img.shields.io/badge/framework-JUCE-orange" alt="JUCE"/>
  <img src="https://img.shields.io/badge/license-GPLv3-green" alt="License"/>
</p>

---

## About

**Open Radio Encoder** is a professional multi-station internet radio streaming encoder. Capture audio from any input device, process it through a 10-band equalizer, compressor, and VST3 plugin chain, then encode and stream to multiple Icecast/Shoutcast servers simultaneously — each with independent codec settings.

Born from a ground-up rewrite of **BUTT (Broadcast Using This Tool)** v1.46.0, Open Radio Encoder rearchitects the streaming engine from a single-server design to a lock-free, multi-threaded pipeline capable of driving N concurrent streams.

---

## Features

- **Multi-station streaming** — Stream to N Icecast/Shoutcast servers simultaneously, each with independent codec, bitrate, and server configuration
- **6 audio codecs** — MP3 (LAME), MP2 (TwoLAME), AAC/HE-AAC (FDK-AAC), Opus (libopus), Vorbis (libvorbis), FLAC (libflac)
- **10-band parametric EQ** + **Dynamic Compressor** — Full DSP chain before encoding
- **VST3 plugin host** — Insert any VST3 plugin into the audio chain (headless processing)
- **Stereo VU meters** — Real-time peak and RMS metering
- **Recording** — Save to WAV, FLAC, AIFF, MP3, MP2, or Opus files locally
- **Auto-reconnect** — Exponential backoff on connection loss (max 10 retries)
- **Scheduler** — Time-based automation of connect/disconnect/record actions
- **Station Manager** — Full CRUD editor for station configurations with JSON persistence
- **Real-time status** — Per-station connection state, stream time, listener count, bytes sent
- **ICY metadata** — Update song titles dynamically on all connected stations
- **CPU load monitor** — Color-coded CPU usage display (green <50%, yellow <80%, red >80%)
- **Custom dark theme** — Professional UI built with JUCE

---

## Screenshots

![Open Radio Encoder](img_obe.png)

---

## Installation

### macOS

Download the latest `OpenRadioEncoder-macOS.dmg` from [Releases](https://github.com/danielrodrigohub/Open-Radio-Encoder/releases), open it, and drag the app to your Applications folder.

### Windows

Download `OpenRadioEncoder-Windows.exe` from [Releases](https://github.com/danielrodrigohub/Open-Radio-Encoder/releases) and run the installer.

### Ubuntu / Debian Linux

Download `OpenRadioEncoder-ubuntu.AppImage` from [Releases](https://github.com/danielrodrigohub/Open-Radio-Encoder/releases), make it executable, and run:

```bash
chmod +x OpenRadioEncoder-ubuntu.AppImage
./OpenRadioEncoder-ubuntu.AppImage
```

### Build from Source

**Requirements:**
- CMake >= 3.22
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- JUCE framework (included as git submodule)
- Dependencies: PortAudio, libmp3lame, twolame, libopus, libvorbis, libogg, libflac, fdk-aac, libshout, libsamplerate, OpenSSL

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/danielrodrigohub/Open-Radio-Encoder.git
cd Open-Radio-Encoder

# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y cmake build-essential \
  libportaudio2 portaudio19-dev \
  libmp3lame-dev libtwolame-dev \
  libopus-dev libvorbis-dev libogg-dev \
  libflac-dev libfdk-aac-dev \
  libshout3-dev libsamplerate0-dev \
  libssl-dev \
  libfreetype-dev libfontconfig-dev \
  libcurl4-openssl-dev libasound2-dev \
  libx11-dev libxcomposite-dev libxext-dev libxrandr-dev \
  libglu1-mesa-dev

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

**macOS (Homebrew):**
```bash
brew install portaudio lame twolame libogg libvorbis opus flac libshout libsamplerate openssl fdk-aac
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

**Windows (vcpkg):**
```bash
vcpkg install portaudio lame twolame opus libvorbis libogg flac fdk-aac libshout libsamplerate openssl
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

---

## CLI Flags

| Flag | Description |
|------|-------------|
| `-h`, `--help` | Show help message and exit |
| `-v`, `--version` | Show version information and exit |
| `-c`, `--config <path>` | Path to `stations.json` config file (default: `<user_app_data>/OpenRadioEncoder/stations.json`) |
| `--headless` | Run without GUI (for server/headless use) |
| `--verbose` | Enable verbose logging |

### Examples

```bash
# Launch with default config
OpenRadioEncoder

# Launch with custom config file
OpenRadioEncoder --config /etc/radio/stations.json

# Launch with verbose logging
OpenRadioEncoder --verbose
```

---

## Architecture

```
PortAudio Callback (RT thread)
    │
    ▼
Ring Buffer ──► Mixer Thread
                   │
                   ├─ Master Gain
                   ├─ 10-band EQ + Compressor
                   ├─ VST3 Plugin Chain
                   ├─ Clamp
                   ├─ VU Meter
                   │
                   ├──► Station #1 Ring Buffer ──► StationConnection #1
                   ├──► Station #2 Ring Buffer ──► StationConnection #2
                   ├──► ...
                   └──► Station #N Ring Buffer ──► StationConnection #N
                                                      │
                                                      ├─ Resample (libsamplerate)
                                                      ├─ Encode (LAME/TwoLAME/FDK-AAC/Opus/Vorbis/FLAC)
                                                      ├─ Send (libshout → Icecast/Shoutcast)
                                                      └─ Auto-reconnect (exponential backoff)
```

---

## Supported Streaming Protocols

| Protocol | Library | TLS |
|----------|---------|-----|
| Icecast (HTTP PUT/SOURCE) | libshout | Yes (OpenSSL) |
| Shoutcast (ICY) | libshout | Yes (OpenSSL) |

---

## Supported Codecs

| Codec | Library | Modes | Container |
|-------|---------|-------|-----------|
| **MP3** | LAME | CBR, VBR, ABR | — |
| **MP2** | TwoLAME | CBR | — |
| **AAC / HE‑AAC** | FDK‑AAC | CBR, VBR | — |
| **Opus** | libopus | VBR | Ogg |
| **Vorbis** | libvorbis | VBR | Ogg |
| **FLAC** | libflac | Lossless | Ogg |
| **WAV** | — | PCM (recording only) | — |

---

## Configuration

Station configurations are persisted as JSON at:

- **macOS**: `~/Library/Application Support/OpenRadioEncoder/stations.json`
- **Windows**: `%APPDATA%/OpenRadioEncoder/stations.json`
- **Linux**: `~/.config/OpenRadioEncoder/stations.json`

Use the `--config` flag to override the path.

---

## License

GPLv3 — See [LICENSE](LICENSE) for details. Open Radio Encoder uses libraries under GPL-compatible licenses (LAME, FDK-AAC, libshout).

---

## Acknowledgments

- **BUTT** (Broadcast Using This Tool) — The original inspiration, by Daniel Nöthen
- **JUCE** — Cross-platform C++ framework by ROLI
- **libshout** — Icecast/Shoutcast streaming library
- All the open-source audio codec projects (LAME, TwoLAME, FDK-AAC, Opus, Vorbis, FLAC)

---

<p align="center">Made in Chile with Love!</p>
