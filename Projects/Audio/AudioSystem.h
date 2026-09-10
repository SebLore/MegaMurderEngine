#pragma once
#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <string>
#include <unordered_map>

enum class TypeAudio : std::uint8_t 
{
    OneShot, 
    Audio3D  
};

class SoundAPI 
{
private:
    // This function is used when we need to adjust the playback speed,
    // such as when walking slower or running faster.
    void PlayEventPerFrame(float deltaTime, const std::string& eventKey);

    // This function is used when no change in playback speed is needed,
    // for example when playing a gunshot sound.
    void PlayOneShot(const std::string& eventKey);

    // Plays a 3D sound event at a specific position (e.g., at the enemy's position).
    void Play3DEvent(const std::string& eventKey, FMOD_VECTOR pos, FMOD_VECTOR fwd, FMOD_VECTOR up);

public:
    SoundAPI();
    ~SoundAPI();

    SoundAPI(const SoundAPI&) = delete;
    SoundAPI& operator=(const SoundAPI&) = delete;

    void Initialize();
    void Reset();
    void Update();

    // Loads a sound bank file from FMOD Studio.
    void LoadBank(const std::string& bankName) const;

    void PlaySound(TypeAudio type,
        const std::string& eventKey,
        FMOD_VECTOR pos = { 0,0,0 }, 
        FMOD_VECTOR fwd = { 0,0,0 },
        FMOD_VECTOR up = { 0,0,0 });

private:
    FMOD::Studio::EventDescription* GetDescription(const std::string& eventKey);
    static std::string GetBankPath(const std::string& bankName);

    FMOD::Studio::System* m_studioSystem = nullptr;
    FMOD::System* m_coreSystem = nullptr;
    
    // Cache to avoid searching the bank every time an event is requested
    std::unordered_map<std::string, FMOD::Studio::EventDescription*> m_eventDescriptions;
};