#include "RecordingEngine.h"
namespace ore {
int RecordingEngine::startRecording(const std::string&, const std::string&, int, int, int) { recording_ = true; return 0; }
void RecordingEngine::stopRecording() { recording_ = false; }
void RecordingEngine::feedAudio(const float*, int, int) {}
} // namespace ore
