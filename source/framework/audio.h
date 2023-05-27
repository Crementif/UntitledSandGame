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
    void Play();
    void Reset();
    void Pause();
    void Update();
    void QueueDestroy();
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

private:
    std::unordered_map<std::string, std::unique_ptr<WavFile>> sounds;
    std::vector<Audio*> voices;
    std::vector<Audio*> destroyQueue;
};