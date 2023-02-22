#include "wav.h"

WavFile::WavFile(std::string path) {
    std::ifstream soundFile((std::string("romfs:/")+path).c_str(), std::ios::in | std::ios::binary);
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
    WavFmtChunkHeader fmtHeader;
    soundFile.read((char*)&fmtHeader, sizeof(WavFmtChunkHeader));
    if (!(fmtHeader.fmt[0] == 'f' && fmtHeader.fmt[1] == 'm' && fmtHeader.fmt[2] == 't')) {
        soundFile.close();
        CriticalErrorHandler("While loading .wav file, didn't find fmt\\0 magic");
    }

    this->bytesPerSample = _swapU16(fmtHeader.bitsPerSample) / 8;
    this->format = _swapU16(fmtHeader.format); // todo: check if format is PCM
    this->channels = _swapU16(fmtHeader.channels);
    if (this->channels != 1 && this->channels != 2) {
        soundFile.close();
        CriticalErrorHandler("While loading .wav file, found a file with more/less then 1-2 channels");
    }
    //soundFile.seekg(sizeof(WavRiffHeader) + (4+4+_swapU32(fmtHeader.subchunkSize)));

    // Read data
    WavDataChunkHeader dataHeader;
    soundFile.read((char*)&dataHeader, sizeof(WavDataChunkHeader));
    if (!(dataHeader.data[0] == 'd' && dataHeader.data[1] == 'a' && dataHeader.data[2] == 't' && dataHeader.data[3] == 'a')) {
        soundFile.close();
        CriticalErrorHandler("While loading .wav file, didn't find data magic");
    }

    //soundFile.seekg(sizeof(WavRiffHeader) + (4+4+_swapU32(fmtHeader.subchunkSize)) + sizeof(WavDataChunkHeader));

    WHBLogPrintf("Load .wav file that's %x bytes long", _swapU32(dataHeader.subchunkSize));
    dataBuffer.resize(_swapU32(dataHeader.subchunkSize), 0);
    soundFile.read((char*)dataBuffer.data(), dataBuffer.size());

    if (this->bytesPerSample == 2) {
        for (uint32_t i=0; i<dataBuffer.size()/this->bytesPerSample; i++) {
            ((uint16_t*)dataBuffer.data())[i] = _swapU16(((uint16_t*)dataBuffer.data())[i]);
        }
    }

    OSReport("Finished loading sample\n");
    soundFile.close();
}