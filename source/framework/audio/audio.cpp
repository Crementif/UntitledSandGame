#include "audio.h"

Audio::Audio(std::string path) {
    const auto& newSound = AudioManager::GetInstance().sounds.try_emplace(path, std::make_unique<OggFile>(path));

    this->m_wavFile = newSound.first->second.get();
    this->m_state = StateEnum::LOADED;
    this->m_path = path;
    AudioManager::GetInstance().voices.emplace_back(this);
}

Audio::~Audio() {
    AudioManager::GetInstance().voices.erase(std::find(AudioManager::GetInstance().voices.begin(), AudioManager::GetInstance().voices.end(), this));
    if (this->m_voiceHandle != nullptr) AXFreeVoice(this->m_voiceHandle);
}

void Audio::SetVolume(uint32_t volume) {
    if (volume > 100)
        volume = 100;

    m_volume = volume;

    if (this->m_voiceHandle == nullptr)
        return;

    AXVoiceVeData voiceData = {
        .volume = (uint16_t)(m_volume * 0x8000 / 100),
        .delta = (int16_t)0x0000
    };
    AXVoiceBegin(this->m_voiceHandle);
    AXSetVoiceVe(this->m_voiceHandle, &voiceData);
    AXVoiceEnd(this->m_voiceHandle);
}


// mario kart 8 calculates the water effect using 3200 as frequency
void Audio::SetLowPassFilter(uint32_t cutoff) {
    if (this->m_voiceHandle == nullptr)
        return;

    int16_t a0, b0;
    AXComputeLpfCoefs(cutoff, &a0, &b0);
    AXPBLPF_t lpf = {
        .on = 1,
        .yn1 = 0,
        .a0 = a0,
        .b0 = b0
    };
    AXSetVoiceLpf(this->m_voiceHandle, &lpf);
    this->SetVolume(100);
}


void Audio::Play() {
    // Handle special states
    if (this->m_state == StateEnum::PLAYING) {
        CriticalErrorHandler("Use additional audio objects for sound effect \"%s\" if you want to play more then one time", this->m_path.c_str());
    }

    if (this->m_state == StateEnum::PAUSED) {
        this->m_state = StateEnum::PLAYING;
        AXVoiceBegin(this->m_voiceHandle);
        AXSetVoiceState(this->m_voiceHandle, AX_VOICE_STATE_PLAYING);
        AXVoiceEnd(this->m_voiceHandle);
        return;
    }

    this->m_voiceHandle = AXAcquireVoice(this->s_priority, nullptr, nullptr);
    if (this->m_voiceHandle == nullptr) {
        WHBLogPrintf("Couldn't play sound due to there not being enough voices!");
        return;
    }
    AXVoiceBegin(this->m_voiceHandle);
    AXSetVoiceType(this->m_voiceHandle, AX_VOICE_TYPE_UNKNOWN);

    // set global audio volume; preferred since per-device mixing is higher quality
    AXVoiceVeData voiceData = {
        .volume = (uint16_t)(m_volume * 0x8000 / 100),
        .delta = (int16_t)0x0000
    };
    AXSetVoiceVe(this->m_voiceHandle, &voiceData);

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
    AXSetVoiceDeviceMix(this->m_voiceHandle, AX_DEVICE_TYPE_TV, 0, mix);
    AXSetVoiceDeviceMix(this->m_voiceHandle, AX_DEVICE_TYPE_DRC, 0, mix);

    this->m_voiceOffsets = {
        .dataType = AX_VOICE_FORMAT_LPCM16, // todo: use wav file to determine audio format
        .loopingEnabled = AX_VOICE_LOOP_DISABLED,
        .loopOffset = 0,
        .endOffset = (uint32_t)this->m_wavFile->m_dataBuffer.size() / 2,
        .currentOffset = 0,
        .data = (const void *)this->m_wavFile->m_dataBuffer.data()
    };
    AXSetVoiceOffsets(this->m_voiceHandle, &this->m_voiceOffsets);

    // todo: don't hardcode 48000
    this->m_voiceSource = {
        .ratio = (uint32_t)(0x00010000 * ((float)48000 / (float)AXGetInputSamplesPerSec())),
        .currentOffsetFrac = 0,
        .lastSample = {0, 0, 0, 0}
    };
    AXSetVoiceSrc(this->m_voiceHandle, &this->m_voiceSource);

    AXSetVoiceSrcType(this->m_voiceHandle, AX_VOICE_SRC_TYPE_LINEAR);
    AXSetVoiceState(this->m_voiceHandle, AX_VOICE_STATE_PLAYING);
    AXVoiceEnd(this->m_voiceHandle);
    this->m_state = StateEnum::PLAYING;
}

void Audio::Pause() {
    if (AXIsVoiceRunning(this->m_voiceHandle)) {
        WHBLogPrint("Can't pause audio that's not running anymore. Maybe check if audio has finished playing!");
    }
    AXSetVoiceState(this->m_voiceHandle, AX_VOICE_STATE_STOPPED);
    this->m_state = StateEnum::PAUSED;
}

void Audio::Reset() {
    AXFreeVoice(this->m_voiceHandle);
    this->m_state = StateEnum::LOADED;
    this->Play();
}

void Audio::Update() {
    if (this->m_voiceHandle == nullptr)
        return;

    if (AXIsVoiceRunning(this->m_voiceHandle)) {
        this->m_state = StateEnum::PLAYING;
    }
    else if (this->m_state == StateEnum::PLAYING) {
        this->m_state = StateEnum::FINISHED;
        AXFreeVoice(this->m_voiceHandle);
        this->m_voiceHandle = nullptr;
    }
}

void Audio::SetLooping(bool looping) {
    if (this->m_voiceHandle == nullptr)
        return;

    AXSetVoiceLoop(this->m_voiceHandle, looping ? AX_VOICE_LOOP_ENABLED : AX_VOICE_LOOP_DISABLED);
}

void Audio::QueueDestroy() {
    AudioManager::GetInstance().destroyQueue.emplace_back(this);
}

void AudioManager::ProcessAudio() {
    for (auto const& voice : this->voices) {
        voice->Update();
    }

    for (auto it = this->destroyQueue.begin(); it != this->destroyQueue.end(); ) {
        if ((*it)->GetState() == Audio::StateEnum::FINISHED) {
            delete (*it);
            it = this->destroyQueue.erase(it);
        }
        else {
            ++it;
        }
    }
}