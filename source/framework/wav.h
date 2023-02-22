#include "../common/common.h"

class WavFile {
public:
    WavFile(std::string path);

    uint32_t channels;
    uint32_t format;
    uint32_t samplingRate; // samples per second
    uint32_t bytesPerSecond; // bytes per second
    uint32_t bytesPerSample; // sampling rate

    std::vector<uint8_t> dataBuffer;
private:
    struct WavRiffHeader {
        uint8_t riff[4];
        uint32_t chunkSize;
        uint8_t wave[4];
    };

    struct WavFmtChunkHeader {
        uint8_t fmt[4];
        uint32_t subchunkSize;
        uint16_t format; // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
        uint16_t channels; // Number of channels 1=Mono 2=Stereo
        uint32_t samplesPerSecond;
        uint32_t bytesPerSecond;
        uint16_t blockAlignment; // 2=16-bit mono, 4=16-bit stereo
        uint16_t bitsPerSample;
    };

    struct WavDataChunkHeader {
        uint8_t data[4];
        uint32_t subchunkSize;
    };
};
