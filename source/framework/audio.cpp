#include "audio.h"

Audio::Audio(std::string path) {
    const auto& newSound = AudioManager::GetInstance().sounds.try_emplace(path, std::make_unique<WavFile>(path));
    this->wavFile = newSound.first->second.get();
    this->state = StateEnum::LOADED;
    AudioManager::GetInstance().voices.emplace_back(this);
}

Audio::~Audio() {
    AudioManager::GetInstance().voices.erase(std::find(AudioManager::GetInstance().voices.begin(), AudioManager::GetInstance().voices.end(), this));
    if (this->voiceHandle != nullptr) AXFreeVoice(this->voiceHandle);
}

void Audio::SetVolume(uint32_t volume) {
    if (volume > 100)
        volume = 100;

    uint32_t volumeConv = ( (0x8000 * volume) / 100 )/100;

    AXVoiceDeviceMixData mix[6] = {
        [0] = {.bus = {[0] = {
                .volume = (uint16_t)(volumeConv >> 16),
                .delta = (int16_t)(volumeConv & 0xFFFF)
        }}},
        [1] = {.bus = {[0] = {
                .volume = (uint16_t)(volumeConv >> 16),
                .delta = (int16_t)(volumeConv & 0xFFFF)
        }}}
    };
    AXSetVoiceDeviceMix(this->voiceHandle, AX_DEVICE_TYPE_TV, 0, mix);
    AXSetVoiceDeviceMix(this->voiceHandle, AX_DEVICE_TYPE_DRC, 0, mix);
}

void Audio::Play() {
    // Handle special states
    if (this->state == StateEnum::PLAYING) {
        CriticalErrorHandler("Use additional audio objects if you want to play more then one time.");
    }

    if (this->state == StateEnum::PAUSED) {
        this->state = StateEnum::PLAYING;
        AXVoiceBegin(this->voiceHandle);
        AXSetVoiceState(this->voiceHandle, AX_VOICE_STATE_PLAYING);
        AXVoiceEnd(this->voiceHandle);
        return;
    }

    this->voiceHandle = AXAcquireVoice(this->priority, nullptr, nullptr);
    if (this->voiceHandle == nullptr) {
        WHBLogPrintf("Couldn't play sound due to there not being enough voices!");
        return;
    }
    AXVoiceBegin(this->voiceHandle);
    AXSetVoiceType(this->voiceHandle, AX_VOICE_TYPE_UNKNOWN);

    // set global audio volume; preferred since per-device mixing is higher quality
    uint32_t globalVolume = 0x80000000;
    AXVoiceVeData voiceData = {
        .volume = (uint16_t)(globalVolume >> 16),
        .delta = (int16_t)(globalVolume & 0xFFFF)
    };
    AXSetVoiceVe(this->voiceHandle, &voiceData);

    // set device audio volume
    AXVoiceDeviceMixData mix[6] = {
            [0] = {.bus = {[0] = {
                    .volume = (uint16_t)(0x80000000 >> 16),
                    .delta = (int16_t)(0x80000000 & 0xFFFF)
            }}},
            [1] = {.bus = {[0] = {
                    .volume = (uint16_t)(0x80000000 >> 16),
                    .delta = (int16_t)(0x80000000 & 0xFFFF)
            }}}
    };
    AXSetVoiceDeviceMix(this->voiceHandle, AX_DEVICE_TYPE_TV, 0, mix);
    AXSetVoiceDeviceMix(this->voiceHandle, AX_DEVICE_TYPE_DRC, 0, mix);

    this->voiceOffsets = {
        .dataType = AX_VOICE_FORMAT_LPCM16, // todo: use wav file to determine audio format
        .loopingEnabled = AX_VOICE_LOOP_DISABLED,
        .loopOffset = 0,
        .endOffset = (uint32_t)this->wavFile->m_dataBuffer.size()/2,
        .currentOffset = 0,
        .data = (const void *)this->wavFile->m_dataBuffer.data()
    };
    AXSetVoiceOffsets(this->voiceHandle, &this->voiceOffsets);

    // todo: don't hardcode 48000
    this->voiceSource = {
        .ratio = (uint32_t)(0x00010000 * ((float)48000 / (float)AXGetInputSamplesPerSec())),
        .currentOffsetFrac = 0,
        .lastSample = {0, 0, 0, 0}
    };
    AXSetVoiceSrc(this->voiceHandle, &this->voiceSource);

    AXSetVoiceSrcType(this->voiceHandle, AX_VOICE_SRC_TYPE_LINEAR);
    AXSetVoiceState(this->voiceHandle, AX_VOICE_STATE_PLAYING);
    AXVoiceEnd(this->voiceHandle);
    this->state = StateEnum::PLAYING;
}

void Audio::Pause() {
    if (AXIsVoiceRunning(this->voiceHandle)) {
        WHBLogPrint("Can't pause audio that's not running anymore. Maybe check if audio has finished playing!");
    }
    AXSetVoiceState(this->voiceHandle, AX_VOICE_STATE_STOPPED);
    this->state = StateEnum::PAUSED;
}

void Audio::Reset() {
    AXFreeVoice(this->voiceHandle);
    this->state = StateEnum::LOADED;
    this->Play();
}

void Audio::Update() {
    if (this->voiceHandle == nullptr) return;
    if (AXIsVoiceRunning(this->voiceHandle)) {
        this->state = StateEnum::PLAYING;
    }
    else if (this->state == StateEnum::PLAYING) {
        this->state = StateEnum::FINISHED;
        AXFreeVoice(this->voiceHandle);
        this->voiceHandle = nullptr;
    }
}

void Audio::QueueDestroy() {
    AudioManager::GetInstance().destroyQueue.emplace_back(this);
}

void AudioManager::ProcessAudio() {
    for (auto const& voice : this->voices) {
        voice->Update();
    }

    for (auto it = this->destroyQueue.rbegin(); it != this->destroyQueue.rend(); ++it) {
        if ((*it)->GetState() == Audio::StateEnum::FINISHED) {
            this->destroyQueue.erase(std::next(it).base());
            delete (*it);
        }
    }
}