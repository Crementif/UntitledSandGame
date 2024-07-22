#include "../../common/common.h"
#include "ogg.h"

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
    void SetLowPassFilter(uint32_t cutoff);
    void Play();
    void Reset();
    void ResetAndPlay();
    void Pause();
    void Stop();
    uint32_t GetOffset();
    void SetOffset(uint32_t offset);



    void SetLooping(bool looping);
    void Update();
    void QueueDestroy();
    [[nodiscard]] StateEnum GetState() const { return m_state; }

private:
    AXVoice* m_voiceHandle = nullptr;
    AXVoiceSrc m_voiceSource;
    AXVoiceOffsets m_voiceOffsets;

    uint32_t m_volume = 100;
    OggFile* m_wavFile = nullptr;
    StateEnum m_state;
    std::string m_path;

    static constexpr uint32_t s_priority = 31;
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
    std::unordered_map<std::string, std::unique_ptr<OggFile>> sounds;
    std::vector<Audio*> voices;
    std::vector<Audio*> destroyQueue;
};