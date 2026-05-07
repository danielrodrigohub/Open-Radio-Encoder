// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — VST3 Host Processor Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "VST3HostProcessor.h"
#include <cstdio>
#include <iostream>

namespace ore {

VST3HostProcessor::VST3HostProcessor() = default;

VST3HostProcessor::~VST3HostProcessor() {
    if (graph_) {
        graph_->clear();
    }
}

void VST3HostProcessor::initialize(double sampleRate, int blockSize, int numChannels) {
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;
    numChannels_ = numChannels;
    
    formatManager_ = std::make_unique<juce::AudioPluginFormatManager>();
    juce::addHeadlessDefaultFormatsToManager(*formatManager_);

    graph_ = std::make_unique<juce::AudioProcessorGraph>();
    graph_->setPlayConfigDetails(numChannels, numChannels, sampleRate, blockSize);
    graph_->prepareToPlay(sampleRate, blockSize);

    // Create I/O nodes
    auto inputNode = graph_->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    auto outputNode = graph_->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    // Connect input directly to output initially
    for (int ch = 0; ch < numChannels; ch++) {
        graph_->addConnection({
            {inputNode->nodeID, ch},
            {outputNode->nodeID, ch}
        });
    }
    
    std::cout << "[VST3Host] Initialized: " << sampleRate << " Hz, " 
              << blockSize << " frames, " << numChannels << " ch" << std::endl;
}

int VST3HostProcessor::loadPlugin(const std::string& vst3Path) {
    if (!formatManager_ || !graph_) return -1;
    
    std::cout << "[VST3Host] Loading plugin: " << vst3Path << std::endl;
    
    juce::String errorMessage;
    
    // Scan the VST3 file
    juce::VST3PluginFormat vst3Format;
    juce::OwnedArray<juce::PluginDescription> results;
    vst3Format.findAllTypesForFile(results, juce::String(vst3Path));

    if (results.isEmpty()) {
        std::cerr << "[VST3Host] No VST3 plugin found in " << vst3Path << std::endl;
        return -1;
    }

    // Create plugin instance
    auto instance = formatManager_->createPluginInstance(
        *results[0], sampleRate_, blockSize_, errorMessage);

    if (!instance) {
        std::cerr << "[VST3Host] Failed to load '" << vst3Path << "': " 
                  << errorMessage.toStdString() << std::endl;
        return -1;
    }

    instance->prepareToPlay(sampleRate_, blockSize_);

    // Suspend audio processing while we change the graph
    graph_->clear(); // Simplification: we just recreate graph for demo purposes
                     // Real implementation would splice it in.
                     
    // Create I/O nodes
    auto inputNode = graph_->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    auto outputNode = graph_->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    // Add as node in the graph
    auto node = graph_->addNode(std::move(instance));

    // Connect input -> plugin -> output
    for (int ch = 0; ch < numChannels_; ch++) {
        graph_->addConnection({{inputNode->nodeID, ch}, {node->nodeID, ch}});
        graph_->addConnection({{node->nodeID, ch}, {outputNode->nodeID, ch}});
    }

    PluginSlot slot;
    slot.nodeId = node->nodeID.uid;
    slot.name = results[0]->name.toStdString();
    slot.path = vst3Path;
    plugins_.push_back(slot);

    return static_cast<int>(plugins_.size()) - 1;
}

void VST3HostProcessor::removePlugin(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < static_cast<int>(plugins_.size())) {
        plugins_.erase(plugins_.begin() + slotIndex);
    }
}

void VST3HostProcessor::setPluginBypassed(int slotIndex, bool bypassed) {
    if (slotIndex >= 0 && slotIndex < static_cast<int>(plugins_.size())) {
        plugins_[slotIndex].bypassed = bypassed;
    }
}

void VST3HostProcessor::processBlock(float* buffer, int numFrames, int numChannels) {
    if (!graph_) return;
    
    juce::AudioBuffer<float> juceBuffer(numChannels, numFrames);
    // De-interleave
    for (int ch = 0; ch < numChannels; ch++) {
        for (int i = 0; i < numFrames; i++) {
            juceBuffer.setSample(ch, i, buffer[i * numChannels + ch]);
        }
    }

    juce::MidiBuffer midi;
    graph_->processBlock(juceBuffer, midi);

    // Re-interleave
    for (int ch = 0; ch < numChannels; ch++) {
        for (int i = 0; i < numFrames; i++) {
            buffer[i * numChannels + ch] = juceBuffer.getSample(ch, i);
        }
    }
}

void VST3HostProcessor::openPluginEditor(int slotIndex) {
    std::cout << "[VST3Host] Opening editor for plugin " << slotIndex << std::endl;
    // Real implementation: create juce::AudioProcessorEditor in a DialogWindow
}

void VST3HostProcessor::closePluginEditor(int slotIndex) {
    std::cout << "[VST3Host] Closing editor for plugin " << slotIndex << std::endl;
}

std::vector<VST3HostProcessor::PluginInfo> VST3HostProcessor::scanDirectory(const std::string& dirPath) {
    std::vector<PluginInfo> results;
    std::cout << "[VST3Host] Scanning: " << dirPath << std::endl;
    // Real implementation: juce::VST3PluginFormat::searchPathsForPlugins()
    return results;
}

} // namespace ore
