#include "../common/common.h"

class WavFile {
public:
    WavFile(std::string path) {
        std::ifstream soundFile(("fs:/vol/content/"+path).c_str(), std::ios::in | std::ios::binary);
        if (!soundFile.is_open())
            CriticalErrorHandler("Failed to open wav file!");

        // Read riff header
        WavRiffHeader riffHeader;
        soundFile.read((char*)&riffHeader, sizeof(WavRiffHeader));
        if (!(riffHeader.riff[0] == 'R' && riffHeader.riff[1] == 'I' && riffHeader.riff[2] == 'F' && riffHeader.riff[3] == 'F')) {
            soundFile.close();
            CriticalErrorHandler("While loading .wav file, didn't find the RIFF magic");
        }
        if (!(riffHeader.wave[0] == 'W' && riffHeader.wave[1] == 'A' && riffHeader.wave[2] == 'V' && riffHeader.wave[3] == 'E')) {
            soundFile.close();
            CriticalErrorHandler("While loading .wav file, didn't find WAVE magic");
        }

        // Read wav format header
        WavFmtChunkHeader fmtHeader = {};
        soundFile.read((char*)&fmtHeader, sizeof(WavFmtChunkHeader));
        if (!(fmtHeader.fmt[0] == 'f' && fmtHeader.fmt[1] == 'm' && fmtHeader.fmt[2] == 't')) {
            soundFile.close();
            CriticalErrorHandler("While loading .wav file, didn't find fmt\\0 magic");
        }

        this->m_samplingRate = _swapU32(fmtHeader.samplesPerSecond);
        if (this->m_samplingRate != 48000) {
            soundFile.close();
            WHBLogPrintf("%s file has a sampling rate of %u, expected 48KHz!", path.c_str(), this->m_samplingRate);
            CriticalErrorHandler("While loading .wav file, found a file with a different sampling rate than 48KHz");
        }

        this->m_bytesPerSecond = _swapU32(fmtHeader.bytesPerSecond);
        this->m_bytesPerSample = _swapU16(fmtHeader.bitsPerSample) / 8;
        this->m_format = _swapU16(fmtHeader.format); // todo: check if format is PCM
        this->m_channels = _swapU16(fmtHeader.channels);
        if (this->m_channels != 1 && this->m_channels != 2) {
            soundFile.close();
            CriticalErrorHandler("While loading .wav file, found a file with more/less then 1-2 channels");
        }
        soundFile.seekg(sizeof(WavRiffHeader) + (4+4+_swapU32(fmtHeader.subchunkSize)));

        // Read chunks until data chunk is found
        WavDataChunkHeader chunkHeader = {};
        do {
            soundFile.read((char*)&chunkHeader, sizeof(WavDataChunkHeader));
            if (!soundFile) {
                soundFile.close();
                CriticalErrorHandler("Failed to read chunk from wav file!");
            }

            // If this is not the 'data' chunk, skip it
            if (!(chunkHeader.data[0] == 'd' && chunkHeader.data[1] == 'a' && chunkHeader.data[2] == 't' && chunkHeader.data[3] == 'a')) {
                soundFile.seekg(_swapU32(chunkHeader.subchunkSize), std::ios::cur); // Skip chunk data
            }

            // Keep reading chunks until we find the 'data' chunk
        } while (!(chunkHeader.data[0] == 'd' && chunkHeader.data[1] == 'a' && chunkHeader.data[2] == 't' && chunkHeader.data[3] == 'a'));

        m_dataBuffer.resize(_swapU32(chunkHeader.subchunkSize), 0);
        soundFile.read((char*)m_dataBuffer.data(), (int32_t)m_dataBuffer.size());

        if (this->m_bytesPerSample == 2) {
            for (uint32_t i=0; i < m_dataBuffer.size() / this->m_bytesPerSample; i++) {
                ((uint16_t*)m_dataBuffer.data())[i] = _swapU16(((uint16_t*)m_dataBuffer.data())[i]);
            }
        }

        WHBLogPrintf("Load .wav file from %s that's %u bytes long", path.c_str(), _swapU32(chunkHeader.subchunkSize));
        soundFile.close();
    }

    uint32_t m_channels;
    uint32_t m_format;
    uint32_t m_samplingRate; // samples per second
    uint32_t m_bytesPerSecond; // bytes per second
    uint32_t m_bytesPerSample; // sampling rate

    std::vector<uint8_t> m_dataBuffer;
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
