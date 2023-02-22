#include "../common/common.h"
#include "wav.h"

class Audio {
    friend class AudioManager;
public:
    enum class StateEnum {
        LOADED,
        PAUSED,
        PLAYING,
        FINISHED
    };

    Audio(std::string path);
    ~Audio();

    void SetVolume(uint32_t volume);
    void SetSampleRate(uint32_t sampleRate);
    void Play();
    void Reset();
    void Pause();
    void Update();
    StateEnum GetState() const { return state; }

private:
    AXVoice* voiceHandle = nullptr;
    AXVoiceSrc voiceSource;
    AXVoiceOffsets voiceOffsets;

    WavFile* wavFile = nullptr;
    StateEnum state;

    static constexpr uint32_t priority = 31;
};

class AudioManager {
    friend class Audio;
private:
    std::unordered_map<std::string, WavFile*> sounds;
    std::vector<Audio*> voices;
public:
    AudioManager() {
        if (!AXIsInit()) {
            AXInitParams params{
                    .renderer = AX_INIT_RENDERER_48KHZ,
                    .pipeline = AX_INIT_PIPELINE_SINGLE
            };
            AXInitWithParams(&params);
        }
        voices = {};
    }
    ~AudioManager() {
        for (auto& it : voices)
            delete it;
        voices.clear();
        for (auto& it : sounds)
            delete it.second;
        sounds.clear();
        if (AXIsInit()) {
            AXQuit();
        }
    }

    void ProcessAudio();

    static AudioManager& GetInstance() {
        static AudioManager sAudioManager;
        return sAudioManager;
    }
};