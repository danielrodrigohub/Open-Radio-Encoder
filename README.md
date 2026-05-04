<div align="center">

# 🎙️ Open Radio Encoder

### Multi-Encoder Profesional para Streaming de Radio por Internet

[![Rust](https://img.shields.io/badge/Rust-1.80+-000000?style=flat&logo=rust&logoColor=white)](https://www.rust-lang.org/)
[![Tauri](https://img.shields.io/badge/Tauri-2.0-FFC131?style=flat&logo=tauri&logoColor=white)](https://tauri.app/)
[![React](https://img.shields.io/badge/React-18-61DAFB?style=flat&logo=react&logoColor=black)](https://react.dev/)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.6-3178C6?style=flat&logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![TailwindCSS](https://img.shields.io/badge/TailwindCSS-3.4-06B6D4?style=flat&logo=tailwindcss&logoColor=white)](https://tailwindcss.com/)
[![Vite](https://img.shields.io/badge/Vite-6.0-646CFF?style=flat&logo=vite&logoColor=white)](https://vitejs.dev/)

[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-808080?style=flat)](https://github.com/danielrodrigohub/Open-Radio-Encoder)
[![License](https://img.shields.io/badge/License-MIT-green?style=flat)](LICENSE)
[![PRs Welcome](https://img.shields.io/badge/PRs-Welcome-brightgreen?style=flat)](https://github.com/danielrodrigohub/Open-Radio-Encoder/pulls)

</div>

---

## 📡 Descripción

**Open Radio Encoder** es un multi-encoder de escritorio para transmisión de radio por internet vía **Icecast** y **Shoutcast**. Construido con **Rust** para el motor de audio de baja latencia y **Tauri + React** para una interfaz moderna en **Dark Mode**, es extremadamente ligero y multiplataforma (Windows, macOS, Linux).

Convierte cualquier fuente de audio del sistema en múltiples streams simultáneos con diferentes códecs, bitrates y destinos, mientras aplicas procesamiento DSP profesional y monitoreas todo en tiempo real con **VU meters** de precisión.

---

## ⚡ Características

### 🎛️ Motor de Audio (Rust)
| Componente | Descripción |
|-----------|-------------|
| **Captura** | Audio de baja latencia desde cualquier dispositivo de entrada del sistema vía `cpal` |
| **Mezclador** | Procesamiento DSP interno con buffer interleaved stereo |
| **EQ 10 Bandas** | 31Hz · 63Hz · 125Hz · 250Hz · 500Hz · 1KHz · 2KHz · 4KHz · 8KHz · 16KHz (±15dB) |
| **Compresor** | Threshold, Ratio, Attack, Release y Makeup Gain configurables |
| **Limitador** | Brickwall limiter con ceiling y release ajustable |
| **VST3** | Soporte para plugins VST3 alojados localmente en la cadena de señal |

### 📻 Multi-Streaming y Códecs
| Códec | Bitrates | Estrategia |
|-------|----------|------------|
| **MP3** | 32–320 kbps | `mp3lame-encoder` (Rust nativo) |
| **AAC / AAC+** | 32–256 kbps | `ffmpeg` + `libfdk_aac` |
| **Opus** | 6–510 kbps | `opus` crate (Rust nativo) |
| **Ogg Vorbis** | Calidad variable | `vorbis-encoder` (Rust nativo) |
| **FLAC** | Lossless | `flac-encoder` (Rust nativo) |
| **WAV** | PCM 16/24-bit | `hound` crate (Rust nativo) |
| **MP2** | 32–384 kbps | `ffmpeg` + `twolame` |

- 📡 **Streaming concurrente** a múltiples servidores Icecast/Shoutcast sincronizados
- 🔄 **Connect / Disconnect** individual por estación o **Connect All** global
- 📊 Monitoreo en tiempo real de **listeners**, **bytes enviados** y **tiempo de stream**

### 🎚️ VU Meters en Tiempo Real
- **4 medidores verticales**: Input L/R + Output L/R
- Rango: **-54 dB** a **0 dB** con escala de colores
- **Peak hold** con decay configurable
- Actualización a **~30 FPS** sin bloquear el hilo principal de la UI
- Comunicación Rust → React vía **Tauri Events** (zero-copy IPC)

### 🏷️ Metadatos Dinámicos
Actualiza el título de la canción automáticamente desde múltiples fuentes:

| Fuente | Descripción |
|--------|-------------|
| 🪟 **Window Title** | Lee el título de cualquier ventana del sistema |
| 📱 **Aplicación** | Monitorea una app específica (Spotify, reproductor) |
| 🌐 **Red** | Metadata desde la red local |
| 📄 **Textfile** | Lee un archivo de texto periódicamente |
| 🔗 **HTTP URL** | Fetch desde una API/webhook remota |

### ⏺️ Grabación
- Graba el **máster de emisión** directamente a disco
- Formatos: **WAV**, **FLAC**, **MP3**
- Muestra el **tamaño del archivo en MB** en tiempo real
- Indicador **REC** con animación de pulso

### 🌐 Control Remoto & Notificaciones
| Funcionalidad | Descripción |
|---------------|-------------|
| 🔌 **HTTP Server** | Micro-servidor local con endpoints `/api/start`, `/api/stop`, `/api/status`, `/api/metadata` |
| 📧 **SMTP** | Notificaciones por correo del estado del stream |
| 🔔 **Webhooks** | Llamadas HTTP a URLs externas en cambios de estado |

---

## 🏗️ Arquitectura

```
┌──────────────────────────────────────────────────────────────┐
│                    FRONTEND (React + TailwindCSS)             │
│  ┌───────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  │
│  │ TopBar    │ │ Stations │ │VU Meters │ │Song/Recording │  │
│  │ Toolbar   │ │  List    │ │ Panel    │ │   Panel       │  │
│  └─────┬─────┘ └────┬─────┘ └────┬─────┘ └───────┬───────┘  │
│        │    invoke() │  invoke()  │                │          │
├────────┼─────────────┼────────────┼────────────────┼──────────┤
│        │    ╔═════════╧════════════╧════════════════╧══════╗  │
│        ╚════╣         TAURI BRIDGE (IPC)                 ║  │
│             ║  Commands ──────────▶ Rust functions       ║  │
│             ║  Events   ◀────────── EngineState (30fps)  ║  │
│             ╚════════════════════════════════════════════╝  │
├──────────────────────────────────────────────────────────────┤
│                    BACKEND (Rust)                             │
│  ┌───────────┐   ┌───────────┐   ┌──────────────────────┐   │
│  │  cpal     │──▶│  Mixer    │──▶│  EncoderManager      │   │
│  │ Capture   │   │ EQ/Comp/  │   │  ┌────────────────┐  │   │
│  │ (44.1kHz) │   │ Limit/VST │   │  │ ffmpeg #1 (MP3)│──┼──▶ Icecast A
│  └─────┬─────┘   └───────────┘   │  │ ffmpeg #2 (AAC)│──┼──▶ Icecast B
│        │                         │  │ ffmpeg #3 (OPUS│──┼──▶ Shoutcast
│        │                         │  └────────────────┘  │   │
│        │   ┌───────────┐         └──────────────────────┘   │
│        └──▶│ VU Meter  │                                     │
│            │ RMS → dBFS│  ┌─────────────────────────────┐   │
│            └───────────┘  │  Remote Server (HTTP 8080)  │   │
│                           │  /api/start  /api/stop      │   │
│                           │  SMTP Notifications         │   │
│                           │  Webhooks                   │   │
│                           └─────────────────────────────┘   │
└──────────────────────────────────────────────────────────────┘
```

### Flujo de Audio
1. `cpal` captura audio del dispositivo de entrada en **F32LE**, **44100 Hz**, **stereo**, buffer de **1024 frames**
2. El buffer viaja por `crossbeam_channel` (ring buffer non-blocking) al loop de procesamiento
3. El **Mixer** aplica la cadena DSP: **EQ 10-bandas → Compresor → Limitador** (y VST3 si están cargados)
4. El **VU Meter** calcula RMS → dBFS en paralelo y emite estado al frontend a 30fps
5. El **EncoderManager** distribuye el audio procesado a `N` instancias de `ffmpeg` (una por estación), cada una con su códec, bitrate y destino Icecast/Shoutcast

---

## 📦 Instalación

### Prerrequisitos

| Plataforma | Dependencias |
|------------|-------------|
| **Windows** | [Microsoft Visual Studio C++ Build Tools](https://visualstudio.microsoft.com/downloads/), [ffmpeg](https://ffmpeg.org/download.html) |
| **macOS** | Xcode Command Line Tools (`xcode-select --install`), ffmpeg (`brew install ffmpeg`) |
| **Linux** | `sudo apt install libwebkit2gtk-4.1-dev libgtk-3-dev libappindicator3-dev libasound2-dev ffmpeg` |

### Instalación Rápida

```bash
# Clonar el repositorio
git clone https://github.com/danielrodrigohub/Open-Radio-Encoder.git
cd Open-Radio-Encoder

# Instalar dependencias frontend
npm install

# Modo desarrollo (hot-reload frontend + backend)
npm run tauri dev

# Build producción
npm run tauri build
```

El binario compilado se encontrará en `src-tauri/target/release/`.

---

## 🚀 Uso

### 1. Seleccionar Dispositivo de Audio
Selecciona el dispositivo de entrada en la barra superior y haz clic en **Start Capture**.

### 2. Configurar Estaciones
Agrega tus servidores Icecast/Shoutcast con:
- **Host** y **Puerto** del servidor
- **Mount point** (ej. `/stream.mp3`)
- **Password** de la fuente
- **Códec** y **Bitrate** deseados

### 3. Conectar
- Haz clic en **Connect** individual para cada estación
- O usa **Connect All** para iniciar todas simultáneamente

### 4. Monitorear
- Observa los **VU Meters** Input/Output en tiempo real
- Verifica **listeners** y **tiempo de stream** por estación
- La **canción actual** se actualiza automáticamente según la fuente de metadatos configurada

### 5. Grabar (Opcional)
Activa **Recording** para guardar el máster de emisión a disco.

---

## 🛠️ Stack Tecnológico

| Capa | Tecnología | Versión |
|------|-----------|---------|
| **Lenguaje Backend** | Rust | 1.80+ |
| **Framework Desktop** | Tauri | 2.x |
| **Captura Audio** | cpal | 0.15 |
| **DSP** | biquad, dasp, rustfft | - |
| **Encoding** | mp3lame-encoder, opus, vorbis-encoder, flac-encoder, hound | - |
| **Streaming** | ffmpeg (child process) | Sistema |
| **Lenguaje Frontend** | TypeScript | 5.6 |
| **UI Framework** | React | 18.x |
| **Estilos** | TailwindCSS | 3.4 |
| **Bundler** | Vite | 6.0 |
| **IPC** | Tauri Commands + Events | 2.x |

---

## 📂 Estructura del Proyecto

```
Open-Radio-Encoder/
├── src/                              # Frontend React + TailwindCSS
│   ├── components/
│   │   ├── TopToolbar.tsx            # Barra superior (dispositivo, captura, settings)
│   │   ├── StationList.tsx           # Panel izquierdo: lista de estaciones
│   │   ├── VUMeterPanel.tsx          # Panel central: VU meters Input/Output L/R
│   │   ├── CurrentSong.tsx           # Sección de canción actual + metadatos
│   │   ├── Recording.tsx             # Sección de grabación (tamaño, formato, REC)
│   │   └── StatusBar.tsx             # Barra inferior: uptime, encoder info
│   ├── styles/
│   │   └── index.css                 # TailwindCSS + estilos globales
│   ├── App.tsx                       # Layout principal, estado, eventos Tauri
│   ├── main.tsx                      # Entry point React
│   └── vite-env.d.ts                 # TypeScript declarations
│
├── src-tauri/                        # Backend Rust
│   ├── src/
│   │   ├── audio/
│   │   │   ├── capture.rs            # Captura cpal, lista dispositivos
│   │   │   ├── vu_meter.rs           # RMS → dBFS, peak hold decay
│   │   │   ├── mixer.rs              # EQ 10-bandas, Compresor, Limitador
│   │   │   ├── encoder.rs            # EncoderManager, ffmpeg orquestación
│   │   │   ├── engine.rs             # AudioEngine: orquesta todo el pipeline
│   │   │   ├── commands.rs           # 14 Tauri Commands (API pública)
│   │   │   └── mod.rs
│   │   ├── server/
│   │   │   ├── mod.rs                # Remote HTTP server, SMTP, Webhooks
│   │   │   └── commands.rs           # Tauri Commands del servidor
│   │   ├── lib.rs                    # Tauri Builder + run()
│   │   └── main.rs                   # Entry point binario
│   ├── Cargo.toml                    # Dependencias Rust
│   ├── tauri.conf.json               # Configuración Tauri
│   └── build.rs                      # Tauri build script
│
├── index.html                        # HTML entry point
├── package.json                      # Dependencias Node.js
├── vite.config.ts                    # Configuración Vite
├── tailwind.config.js                # Configuración TailwindCSS
├── postcss.config.js                 # PostCSS
├── tsconfig.json                     # TypeScript config
└── README.md                         # Este archivo
```

---

## 🔧 Desarrollo

```bash
# Instalar dependencias
npm install

# Desarrollo con hot-reload
npm run tauri dev

# Linting y typecheck
npm run build

# Build release final
npm run tauri build
```

### Variables de Entorno

| Variable | Descripción | Default |
|----------|-------------|---------|
| `TAURI_DEV_HOST` | Host para HMR en desarrollo | `localhost` |
| `RUST_LOG` | Nivel de logging Rust | `info` |

---

## 📄 Licencia

MIT © 2026 [Daniel Rodrigo](https://github.com/danielrodrigohub)

---

<div align="center">

### ⭐ Dale una estrella si te gusta el proyecto

**[Reportar Bug](https://github.com/danielrodrigohub/Open-Radio-Encoder/issues)** ·
**[Solicitar Feature](https://github.com/danielrodrigohub/Open-Radio-Encoder/issues)** ·
**[Contribuir](https://github.com/danielrodrigohub/Open-Radio-Encoder/pulls)**

</div>
