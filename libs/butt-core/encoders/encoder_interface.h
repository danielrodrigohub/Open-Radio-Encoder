// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Encoder Interface
// Abstract base class for all audio codecs
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include <cstdint>
#include <string>

namespace ore {

/// Supported codec types
enum class CodecType {
    MP3,
    MP2,    // NEW: via twolame
    AAC,
    Opus,
    Vorbis,
    FLAC,
    WAV
};

/// Encoder configuration
struct EncoderConfig {
    CodecType codec       = CodecType::MP3;
    int       samplerate  = 44100;
    int       channels    = 2;       // 1=mono, 2=stereo
    int       bitrate     = 128;     // kbps
    int       quality     = 5;       // encoder-specific quality (0=best, 9=fastest for LAME)
    int       bitrate_mode = 0;      // 0=CBR, 1=VBR, 2=ABR

    // Content-type for HTTP headers
    std::string contentType() const {
        switch (codec) {
            case CodecType::MP3:    return "audio/mpeg";
            case CodecType::MP2:    return "audio/mpeg";
            case CodecType::AAC:    return "audio/aac";
            case CodecType::Opus:   return "audio/ogg";
            case CodecType::Vorbis: return "audio/ogg";
            case CodecType::FLAC:   return "audio/ogg";
            default:                return "audio/mpeg";
        }
    }

    std::string codecName() const {
        switch (codec) {
            case CodecType::MP3:    return "MP3";
            case CodecType::MP2:    return "MP2";
            case CodecType::AAC:    return "AAC";
            case CodecType::Opus:   return "Opus";
            case CodecType::Vorbis: return "Vorbis";
            case CodecType::FLAC:   return "FLAC";
            case CodecType::WAV:    return "WAV";
            default:                return "Unknown";
        }
    }
};

/// Abstract encoder interface
/// Each codec (MP3, MP2, AAC, Opus, Vorbis, FLAC) implements this interface.
/// Instances are NOT thread-safe — each station thread owns its own encoder.
class IEncoder {
public:
    virtual ~IEncoder() = default;

    /// Initialize the encoder with the given configuration.
    /// Returns 0 on success, non-zero on failure.
    virtual int init(const EncoderConfig& config) = 0;

    /// Reinitialize the encoder (close + init).
    virtual int reinit(const EncoderConfig& config) {
        close();
        return init(config);
    }

    /// Encode interleaved float32 PCM samples.
    /// @param pcm_in       Interleaved float32 input samples [-1.0, 1.0]
    /// @param num_samples  Number of samples PER CHANNEL
    /// @param enc_out      Output buffer for encoded data
    /// @param max_out_bytes  Size of output buffer
    /// @return Number of encoded bytes written, or negative on error
    virtual int encode(const float* pcm_in, int num_samples,
                       uint8_t* enc_out, int max_out_bytes) = 0;

    /// Flush any remaining encoded data (call before closing a file).
    /// @return Number of bytes flushed
    virtual int flush(uint8_t* enc_out, int max_out_bytes) { return 0; }

    /// Close and free encoder resources.
    virtual void close() = 0;

    /// Get the codec type
    virtual CodecType type() const = 0;

    /// Get the actual output samplerate (may differ from input, e.g. Opus forces 48000)
    virtual int outputSamplerate() const { return config_.samplerate; }

protected:
    EncoderConfig config_;
};

} // namespace ore
