// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — VST3 Host Processor
// Loads and processes VST3 plugins using JUCE's AudioProcessorGraph
//
// This is a NEW capability not present in BUTT.
//
// Architecture:
//   The VST3HostProcessor wraps a JUCE AudioProcessorGraph.
//   VST3 plugins are loaded as nodes in the graph, connected in series.
//   The processBlock() method is called from the mixer thread,
//   processing the master audio buffer in-place.
//
// Integration point in the pipeline:
//   PortAudio → Mixer → DSP (EQ/Comp) → [VST3HostProcessor] → Distributor
//
// Usage:
//   vst3Host.loadPlugin("/path/to/plugin.vst3");
//   vst3Host.prepareToPlay(sampleRate, bufferSize);
//   // In mixer loop:
//   vst3Host.processBlock(audioBuffer, midiBuffer);
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declarations (avoid including JUCE in this header)
// When building with JUCE, these will resolve to real types.
// When building standalone butt-core, this file is not compiled.

namespace juce {
    class AudioProcessorGraph;
    class AudioPluginFormatManager;
    class AudioProcessorEditor;
    class AudioBuffer;
    class MidiBuffer;
}

namespace ore {

struct PluginSlot {
    int         nodeId = -1;
    std::string name;
    std::string path;
    bool        bypassed = false;
};

/// VST3 Host Processor — manages a chain of VST3 plugins
///
/// This leverages JUCE's AudioProcessorGraph to create a flexible
/// plugin chain that processes audio in the mixer thread.
///
/// Example JUCE integration:
/// ```cpp
/// // In AudioPipeline::mixerLoop():
/// juce::AudioBuffer<float> juceBuffer(buffer, channels, frames);
/// juce::MidiBuffer midi;
/// vst3Host_->processBlock(juceBuffer, midi);
/// ```
class VST3HostProcessor {
public:
    VST3HostProcessor();
    ~VST3HostProcessor();

    /// Initialize the plugin format manager and graph.
    void initialize(double sampleRate, int blockSize, int numChannels);

    /// Load a VST3 plugin from a file path.
    /// Returns the slot index, or -1 on failure.
    int loadPlugin(const std::string& vst3Path);

    /// Remove a plugin from the chain.
    void removePlugin(int slotIndex);

    /// Bypass/enable a plugin.
    void setPluginBypassed(int slotIndex, bool bypassed);

    /// Get the list of loaded plugins.
    const std::vector<PluginSlot>& plugins() const { return plugins_; }

    /// Process a block of audio through the plugin chain.
    /// Called from the mixer thread.
    /// @param buffer  Audio buffer (modified in-place)
    /// @param numFrames  Number of frames
    /// @param numChannels  Number of channels
    void processBlock(float* buffer, int numFrames, int numChannels);

    /// Open the plugin's native editor GUI in a floating window.
    void openPluginEditor(int slotIndex);

    /// Close the plugin editor.
    void closePluginEditor(int slotIndex);

    /// Scan a directory for VST3 plugins.
    struct PluginInfo {
        std::string name;
        std::string path;
        std::string manufacturer;
    };
    static std::vector<PluginInfo> scanDirectory(const std::string& dirPath);

private:
    std::vector<PluginSlot> plugins_;
    double sampleRate_ = 44100.0;
    int blockSize_ = 1024;
    int numChannels_ = 2;

    // JUCE objects (only available when building with JUCE)
    // In production, these would be:
    //   std::unique_ptr<juce::AudioProcessorGraph> graph_;
    //   std::unique_ptr<juce::AudioPluginFormatManager> formatManager_;
};

} // namespace ore
