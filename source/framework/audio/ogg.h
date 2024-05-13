#define STB_VORBIS_HEADER_ONLY
#include "../../common/stb_vorbis.c"

class OggFile {
public:
    OggFile(std::string path) {
        std::ifstream soundFile(("fs:/vol/content/" + path).c_str(), std::ios::in | std::ios::binary);
        if (!soundFile.is_open())
            CriticalErrorHandler("Failed to open ogg file!");

        // Read the file into a memory buffer
        soundFile.seekg(0, std::ios::end);
        size_t fileSize = soundFile.tellg();
        soundFile.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileBuffer(fileSize);
        soundFile.read((char*)fileBuffer.data(), fileSize);
        soundFile.close();

        // Decode the OGG file using stb_vorbis
        int channels, sampleRate;
        short* output;
        int samples = stb_vorbis_decode_memory(fileBuffer.data(), fileSize, &channels, &sampleRate, &output);
        if (samples == -1)
            CriticalErrorHandler("Failed to decode ogg file!");

        if (sampleRate != 48000) {
            free(output);
            WHBLogPrintf("%s file has a sampling rate of %u, expected 48KHz!", path.c_str(), sampleRate);
            CriticalErrorHandler("While loading .ogg file, found a file with a different sampling rate than 48KHz");
        }

        this->m_channels = channels;
        this->m_samplingRate = sampleRate;
        this->m_bytesPerSample = 2; // 16-bit samples
        this->m_dataBuffer.assign((uint8_t*)output, (uint8_t*)(output + samples * channels));
        free(output);

        WHBLogPrintf("Load .ogg file from %s that's %zu bytes long", path.c_str(), m_dataBuffer.size());
    }

    uint32_t m_channels;
    uint32_t m_samplingRate; // samples per second
    uint32_t m_bytesPerSample; // bytes per sample
    std::vector<uint8_t> m_dataBuffer;
};
