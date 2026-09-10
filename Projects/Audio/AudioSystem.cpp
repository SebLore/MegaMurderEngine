#include "AudioSystem.h"
#include <iostream>
#include <filesystem>

namespace {
    bool CheckError(FMOD_RESULT result, const char* msg) {
        if (result != FMOD_OK) {
            std::cerr << "FMOD Error: " << msg << '\n';
            return false;
        }
        return true;
    }
}

SoundAPI::SoundAPI() { Initialize(); }
SoundAPI::~SoundAPI() { Reset(); }

//Constructor and Destructor 
void SoundAPI::Initialize() {
    CheckError(FMOD::Studio::System::create(&m_studioSystem), "Create System");
    CheckError(m_studioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr), "Init System");
    m_studioSystem->getCoreSystem(&m_coreSystem);
}

void SoundAPI::Reset() {
    m_eventDescriptions.clear();
    m_studioSystem->unloadAll();

    if (m_studioSystem) {
        m_studioSystem->release();
        m_studioSystem = nullptr;
        m_coreSystem = nullptr;
    }
}

//Update
void SoundAPI::Update() {
    if (m_studioSystem) m_studioSystem->update();
}

//Decription
FMOD::Studio::EventDescription* SoundAPI::GetDescription(const std::string& eventKey) {
    if (m_eventDescriptions.count(eventKey)) return m_eventDescriptions[eventKey];

    std::string path = "event:/" + eventKey;
    FMOD::Studio::EventDescription* desc = nullptr;
    if (CheckError(m_studioSystem->getEvent(path.c_str(), &desc), eventKey.c_str())) {
        m_eventDescriptions[eventKey] = desc;
        return desc;
    }
    return nullptr;
}

//Load bank
void SoundAPI::LoadBank(const std::string& bankName) const {
    FMOD::Studio::Bank* bank = nullptr;
    m_studioSystem->loadBankFile(GetBankPath(bankName + ".bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
    m_studioSystem->loadBankFile(GetBankPath(bankName + ".strings.bank").c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
}

std::string SoundAPI::GetBankPath(const std::string& bankName) {
    auto path = std::filesystem::current_path() / ".." / "assets" / "audio" / "soundBank" / "Build" / "Desktop" / bankName;
    return std::filesystem::canonical(path).string();
}

//Event
void SoundAPI::PlayOneShot(const std::string& eventKey) {
    auto* desc = GetDescription(eventKey);
    if (desc) {

        FMOD::Studio::EventInstance* instance = nullptr;
        desc->createInstance(&instance);
/*        int length;
        desc->getLength(&length);
        int boring = length*/;

        instance->start();
        instance->release();
    }
}

void SoundAPI::Play3DEvent(const std::string& eventKey, FMOD_VECTOR pos, FMOD_VECTOR fwd, FMOD_VECTOR up)
{
    FMOD::Studio::EventDescription* eventDesc = GetDescription(eventKey);

    if (eventDesc)
    {
        FMOD::Studio::EventInstance* instance = nullptr;
        FMOD_RESULT result = eventDesc->createInstance(&instance);

        if (CheckError(result, "Could not create 3D instance"))
        {
            FMOD_3D_ATTRIBUTES attributes = { { 0 } }; //<- This one reset to zero
            attributes.position = pos;
            attributes.forward = fwd;
            attributes.up = up;
            attributes.velocity = { 0, 0, 0 };

            instance->set3DAttributes(&attributes);

            instance->start();
            instance->release();
        }
    }
}

void SoundAPI::PlaySound(TypeAudio type, const std::string& eventKey,
    FMOD_VECTOR pos, FMOD_VECTOR fwd, FMOD_VECTOR up)
{
    switch (type) {
    case TypeAudio::OneShot:
        PlayOneShot(eventKey);
        break;

    case TypeAudio::Audio3D:
        Play3DEvent(eventKey, pos, fwd, up);
        break;

    default:
        std::cerr << "Unknown audio type!" << std::endl;
        break;
    }
}